use std::str::FromStr;

use crate::error::SkError;

const DEFAULT_FONT: &str = "sans-serif";

/// The minimum font-weight value per:
///
/// https://drafts.csswg.org/css-fonts-4/#font-weight-numeric-values
pub const MIN_FONT_WEIGHT: f32 = 1.;

/// The maximum font-weight value per:
///
/// https://drafts.csswg.org/css-fonts-4/#font-weight-numeric-values
pub const MAX_FONT_WEIGHT: f32 = 1000.;

/// The default font size.
pub const FONT_MEDIUM_PX: f32 = 16.0;

#[derive(Debug, Clone, PartialEq)]
pub struct Font {
  pub size: f32,
  pub style: FontStyle,
  pub family: String,
  pub variant: FontVariant,
  pub stretch: FontStretch,
  pub weight: u32,
}

impl Default for Font {
  fn default() -> Self {
    Font {
      size: 10.0,
      style: FontStyle::Normal,
      family: DEFAULT_FONT.to_owned(),
      variant: FontVariant::Normal,
      stretch: FontStretch::Normal,
      weight: 400,
    }
  }
}

impl Font {
  // CSS shorthand grammar:
  //   [ <font-style> || <font-variant-css2> || <font-weight> || <font-stretch> ]?
  //     <font-size> [/<line-height>]? <font-family>
  //
  // Descriptors appear in any order before the size token. Each descriptor may
  // appear at most once. The literal `normal` is the initial value of every
  // descriptor, so it is accepted but ignored — treating it as style would
  // overwrite an earlier explicit keyword (this was the previous regex's bug
  // with `italic normal 48px Foo`). A descriptor's value-set uniquely identifies
  // it except for `normal` and `NN%` (which can be either a stretch percentage
  // or the size itself). The percentage ambiguity is resolved by lookahead: a
  // numeric-leading token that has no subsequent numeric-leading token is the
  // size; otherwise it is a descriptor candidate.
  pub fn new(font_rules: &str) -> Result<Font, SkError> {
    let input = font_rules.trim();
    if input.is_empty() {
      return Err(SkError::InvalidFontStyle(font_rules.to_owned()));
    }

    let default_font = Font::default();
    let mut style: Option<FontStyle> = None;
    let mut variant: Option<FontVariant> = None;
    let mut weight: Option<u32> = None;
    let mut stretch: Option<FontStretch> = None;

    let bytes = input.as_bytes();
    let mut cursor = 0usize;

    let size_token = loop {
      while cursor < bytes.len() && bytes[cursor].is_ascii_whitespace() {
        cursor += 1;
      }
      if cursor >= bytes.len() {
        return Err(SkError::InvalidFontStyle(font_rules.to_owned()));
      }
      let token_start = cursor;
      while cursor < bytes.len() && !bytes[cursor].is_ascii_whitespace() {
        cursor += 1;
      }
      let token = &input[token_start..cursor];

      if token == "normal" {
        // `normal` is accepted by every descriptor; committing it would overwrite
        // an earlier explicit keyword. Since every descriptor's default is already
        // normal, skipping the token is equivalent to setting whichever descriptor
        // was intended without clobbering ones already assigned.
        continue;
      }

      let numeric_leading = token
        .chars()
        .next()
        .is_some_and(|c| c.is_ascii_digit() || c == '.');

      if numeric_leading {
        // A numeric-leading token is a descriptor candidate only if another
        // numeric-leading token follows — the final one is the size.
        if !has_following_numeric_token(&input[cursor..]) {
          break (token_start, cursor);
        }
        if token.ends_with('%') && stretch.is_none() {
          if let Some(s) = parse_font_stretch(token) {
            stretch = Some(s);
          }
          // Unrecognized percentages (e.g. "52%") are consumed without effect,
          // matching prior regex behavior.
          continue;
        }
        if weight.is_none()
          && let Some(w) = parse_font_weight(token)
        {
          weight = Some(w);
          continue;
        }
        // Numeric token with no applicable unassigned descriptor: treat as size.
        break (token_start, cursor);
      }

      if style.is_none()
        && let Ok(s) = FontStyle::from_str(token)
      {
        // `normal` handled above; only italic/oblique reach here.
        style = Some(s);
        continue;
      }
      if variant.is_none() && token == "small-caps" {
        variant = Some(FontVariant::SmallCaps);
        continue;
      }
      if weight.is_none()
        && let Some(w) = parse_font_weight(token)
      {
        weight = Some(w);
        continue;
      }
      if stretch.is_none()
        && let Some(s) = parse_font_stretch(token)
      {
        stretch = Some(s);
        continue;
      }

      // Unrecognized non-numeric token before a size was found: malformed input.
      return Err(SkError::InvalidFontStyle(font_rules.to_owned()));
    };

    let size_raw = &input[size_token.0..size_token.1];
    let size_part = match size_raw.find('/') {
      Some(i) => &size_raw[..i],
      None => size_raw,
    };
    let (size_num, size_unit) = parse_number_with_unit(size_part)
      .ok_or_else(|| SkError::InvalidFontStyle(font_rules.to_owned()))?;
    let unit = size_unit.unwrap_or("px");
    let size_input = if unit == "%" {
      size_num / 100.0 * FONT_MEDIUM_PX
    } else {
      size_num
    };
    let size_px = parse_size_px(size_input, unit);

    let family_str = input[size_token.1..].trim();
    let family = if family_str.is_empty() {
      default_font.family.clone()
    } else {
      family_str
        .split(',')
        .map(|s| s.trim())
        .map(strip_family_quotes)
        .collect::<Vec<&str>>()
        .join(",")
    };

    Ok(Font {
      size: size_px,
      style: style.unwrap_or(default_font.style),
      variant: variant.unwrap_or(default_font.variant),
      weight: weight.unwrap_or(default_font.weight),
      stretch: stretch.unwrap_or(default_font.stretch),
      family,
    })
  }
}

// Lookahead used to disambiguate stretch-percentage vs size-percentage. If a
// later whitespace-separated token starts with a digit or '.', the current
// numeric-leading token is not the size.
fn has_following_numeric_token(rest: &str) -> bool {
  rest
    .split_ascii_whitespace()
    .next()
    .and_then(|t| t.chars().next())
    .is_some_and(|c| c.is_ascii_digit() || c == '.')
}

// Extracts (number, unit) from a size token like "48", "48px", "50%", "1.2em".
// Returns None for tokens that do not begin with a parseable decimal number.
fn parse_number_with_unit(token: &str) -> Option<(f32, Option<&str>)> {
  let unit_start = token
    .find(|c: char| !c.is_ascii_digit() && c != '.')
    .unwrap_or(token.len());
  if unit_start == 0 {
    return None;
  }
  let num: f32 = token[..unit_start].parse().ok()?;
  let unit = if unit_start == token.len() {
    None
  } else {
    Some(&token[unit_start..])
  };
  Some((num, unit))
}

fn strip_family_quotes(s: &str) -> &str {
  if s.len() >= 2
    && ((s.starts_with('"') && s.ends_with('"')) || (s.starts_with('\'') && s.ends_with('\'')))
  {
    &s[1..s.len() - 1]
  } else {
    s
  }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FontStyle {
  Normal,
  Italic,
  Oblique,
}

impl FontStyle {
  pub fn as_str(&self) -> &str {
    match *self {
      Self::Italic => "italic",
      Self::Normal => "normal",
      Self::Oblique => "oblique",
    }
  }
}

impl FromStr for FontStyle {
  type Err = SkError;

  fn from_str(s: &str) -> Result<FontStyle, SkError> {
    match s {
      "normal" => Ok(Self::Normal),
      "italic" => Ok(Self::Italic),
      "oblique" => Ok(Self::Oblique),
      _ => Err(SkError::InvalidFontStyle(s.to_owned())),
    }
  }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum FontVariant {
  Normal,
  SmallCaps,
}

impl FromStr for FontVariant {
  type Err = SkError;

  fn from_str(s: &str) -> Result<FontVariant, SkError> {
    match s {
      "normal" => Ok(Self::Normal),
      "small-caps" => Ok(Self::SmallCaps),
      _ => Err(SkError::InvalidFontVariant(s.to_owned())),
    }
  }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FontStretch {
  UltraCondensed = 1,
  ExtraCondensed = 2,
  Condensed = 3,
  SemiCondensed = 4,
  Normal = 5,
  SemiExpanded = 6,
  Expanded = 7,
  ExtraExpanded = 8,
  UltraExpanded = 9,
}

impl From<i32> for FontStretch {
  fn from(value: i32) -> Self {
    match value {
      1 => FontStretch::UltraCondensed,
      2 => FontStretch::ExtraCondensed,
      3 => FontStretch::Condensed,
      4 => FontStretch::SemiCondensed,
      5 => FontStretch::Normal,
      6 => FontStretch::SemiExpanded,
      7 => FontStretch::Expanded,
      8 => FontStretch::ExtraExpanded,
      9 => FontStretch::UltraExpanded,
      _ => unreachable!(),
    }
  }
}

impl FontStretch {
  pub fn as_str(&self) -> &str {
    match *self {
      FontStretch::UltraCondensed => "ultra-condensed",
      FontStretch::ExtraCondensed => "extra-condensed",
      FontStretch::Condensed => "condensed",
      FontStretch::SemiCondensed => "semi-condensed",
      FontStretch::Normal => "normal",
      FontStretch::SemiExpanded => "semi-expanded",
      FontStretch::Expanded => "expanded",
      FontStretch::ExtraExpanded => "extra-expanded",
      FontStretch::UltraExpanded => "ultra-expanded",
    }
  }

  /// Returns the width percentage for variable font 'wdth' axis
  /// Based on CSS font-stretch percentages
  pub fn to_width_percentage(self) -> f32 {
    match self {
      FontStretch::UltraCondensed => 50.0,
      FontStretch::ExtraCondensed => 62.5,
      FontStretch::Condensed => 75.0,
      FontStretch::SemiCondensed => 87.5,
      FontStretch::Normal => 100.0,
      FontStretch::SemiExpanded => 112.5,
      FontStretch::Expanded => 125.0,
      FontStretch::ExtraExpanded => 150.0,
      FontStretch::UltraExpanded => 200.0,
    }
  }
}

// https://drafts.csswg.org/css-fonts-4/#propdef-font-weight
fn parse_font_weight(weight: &str) -> Option<u32> {
  match weight {
    "lighter" | "100" => Some(100),
    "200" => Some(200),
    "300" => Some(300),
    "normal" | "400" => Some(400),
    "500" => Some(500),
    "600" => Some(600),
    "bold" | "bolder" | "700" => Some(700),
    _ => weight.parse::<f32>().ok().and_then(|w| {
      if (MIN_FONT_WEIGHT..=MAX_FONT_WEIGHT).contains(&w) {
        Some(w as u32)
      } else {
        None
      }
    }),
  }
}

pub fn parse_font_stretch(stretch: &str) -> Option<FontStretch> {
  match stretch {
    "ultra-condensed" | "50%" => Some(FontStretch::UltraCondensed),
    "extra-condensed" | "62.5%" => Some(FontStretch::ExtraCondensed),
    "condensed" | "75%" => Some(FontStretch::Condensed),
    "semi-condensed" | "87.5%" => Some(FontStretch::SemiCondensed),
    "normal" | "100%" => Some(FontStretch::Normal),
    "semi-expanded" | "112.5%" => Some(FontStretch::SemiExpanded),
    "expanded" | "125%" => Some(FontStretch::Expanded),
    "extra-expanded" | "150%" => Some(FontStretch::ExtraExpanded),
    "ultra-expanded" | "200%" => Some(FontStretch::UltraExpanded),
    _ => None,
  }
}

pub fn parse_size_px(size: f32, unit: &str) -> f32 {
  let mut size_px = size;
  match unit {
    "em" | "rem" | "pc" => {
      size_px = size * FONT_MEDIUM_PX;
    }
    "pt" => {
      size_px = size * 4.0 / 3.0;
    }
    "px" => {
      size_px = size;
    }
    "in" => {
      size_px = size * 96.0;
    }
    "cm" => {
      size_px = size * 96.0 / 2.54;
    }
    "mm" => {
      size_px = size * 96.0 / 25.4;
    }
    "q" => {
      size_px = size * 96.0 / 25.4 / 4.0;
    }
    "%" => {
      size_px = size * FONT_MEDIUM_PX / 100.0;
    }
    _ => {}
  };
  size_px
}

#[test]
fn font_stretch() {
  assert_eq!(
    parse_font_stretch("ultra-condensed"),
    Some(FontStretch::UltraCondensed)
  );
  assert_eq!(parse_font_stretch("50%"), Some(FontStretch::UltraCondensed));
  assert_eq!(
    parse_font_stretch("extra-condensed"),
    Some(FontStretch::ExtraCondensed)
  );
  assert_eq!(
    parse_font_stretch("62.5%"),
    Some(FontStretch::ExtraCondensed)
  );
  assert_eq!(
    parse_font_stretch("condensed"),
    Some(FontStretch::Condensed)
  );
  assert_eq!(parse_font_stretch("75%"), Some(FontStretch::Condensed));
  assert_eq!(
    parse_font_stretch("semi-condensed"),
    Some(FontStretch::SemiCondensed)
  );
  assert_eq!(
    parse_font_stretch("87.5%"),
    Some(FontStretch::SemiCondensed)
  );
  assert_eq!(parse_font_stretch("normal"), Some(FontStretch::Normal));
  assert_eq!(parse_font_stretch("100%"), Some(FontStretch::Normal));
  assert_eq!(
    parse_font_stretch("semi-expanded"),
    Some(FontStretch::SemiExpanded)
  );
  assert_eq!(
    parse_font_stretch("112.5%"),
    Some(FontStretch::SemiExpanded)
  );
  assert_eq!(parse_font_stretch("expanded"), Some(FontStretch::Expanded));
  assert_eq!(parse_font_stretch("125%"), Some(FontStretch::Expanded));
  assert_eq!(
    parse_font_stretch("extra-expanded"),
    Some(FontStretch::ExtraExpanded)
  );
  assert_eq!(parse_font_stretch("150%"), Some(FontStretch::ExtraExpanded));
  assert_eq!(
    parse_font_stretch("ultra-expanded"),
    Some(FontStretch::UltraExpanded)
  );
  assert_eq!(parse_font_stretch("200%"), Some(FontStretch::UltraExpanded));
  assert_eq!(parse_font_stretch("52%"), None);
  assert_eq!(parse_font_stretch("-50%"), None);
  assert_eq!(parse_font_stretch("50"), None);
  assert_eq!(parse_font_stretch("ultra"), None);
}

#[test]
fn test_parse_font_weight() {
  assert_eq!(parse_font_weight("lighter"), Some(100));
  assert_eq!(parse_font_weight("normal"), Some(400));
  assert_eq!(parse_font_weight("bold"), Some(700));
  assert_eq!(parse_font_weight("bolder"), Some(700));
  assert_eq!(parse_font_weight("100"), Some(100));
  assert_eq!(parse_font_weight("100.1"), Some(100));
  assert_eq!(parse_font_weight("120"), Some(120));
  assert_eq!(parse_font_weight("0.01"), None);
  assert_eq!(parse_font_weight("-20"), None);
  assert_eq!(parse_font_weight("whatever"), None);
}

#[allow(clippy::float_cmp)]
#[test]
fn test_parse_size_px() {
  assert_eq!(parse_size_px(12.0, "px"), 12.0f32);
  assert_eq!(parse_size_px(2.0, "em"), 32.0f32);
}

// Covers the parser bug where `normal` following an explicit style/weight/
// stretch keyword was silently overwriting the earlier keyword, making
// `italic normal 48px F` render identically to `normal normal 48px F`.
#[test]
fn test_font_shorthand_normal_does_not_overwrite() {
  let cases: &[(&str, FontStyle, u32, FontStretch, FontVariant)] = &[
    (
      "italic normal 48px Foo",
      FontStyle::Italic,
      400,
      FontStretch::Normal,
      FontVariant::Normal,
    ),
    (
      "italic normal normal 48px Foo",
      FontStyle::Italic,
      400,
      FontStretch::Normal,
      FontVariant::Normal,
    ),
    (
      "normal italic 48px Foo",
      FontStyle::Italic,
      400,
      FontStretch::Normal,
      FontVariant::Normal,
    ),
    (
      "oblique bold normal 48px Foo",
      FontStyle::Oblique,
      700,
      FontStretch::Normal,
      FontVariant::Normal,
    ),
    (
      "bold normal condensed italic 48px Foo",
      FontStyle::Italic,
      700,
      FontStretch::Condensed,
      FontVariant::Normal,
    ),
    (
      "small-caps normal 48px Foo",
      FontStyle::Normal,
      400,
      FontStretch::Normal,
      FontVariant::SmallCaps,
    ),
  ];
  for (rule, style, weight, stretch, variant) in cases {
    let font = Font::new(rule).unwrap();
    assert_eq!(font.style, *style, "style for rule = {rule:?}");
    assert_eq!(font.weight, *weight, "weight for rule = {rule:?}");
    assert_eq!(font.stretch, *stretch, "stretch for rule = {rule:?}");
    assert_eq!(font.variant, *variant, "variant for rule = {rule:?}");
  }
}

#[test]
fn test_font_new() {
  let fixtures: Vec<(&'static str, Font)> = vec![
    (
      "20px Arial",
      Font {
        size: 20.0,
        family: "Arial".to_owned(),
        ..Default::default()
      },
    ),
    (
      "20pt Arial",
      Font {
        size: 26.666_666,
        family: "Arial".to_owned(),
        ..Default::default()
      },
    ),
    (
      "20.5pt Arial",
      Font {
        size: 27.333_334,
        family: "Arial".to_owned(),
        ..Default::default()
      },
    ),
    (
      "50% Arial",
      Font {
        size: 8.0,
        family: "Arial".to_owned(),
        ..Default::default()
      },
    ),
    (
      "62.5% 50% Arial",
      Font {
        size: 8.0,
        family: "Arial".to_owned(),
        stretch: FontStretch::ExtraCondensed,
        ..Default::default()
      },
    ),
    (
      "20mm Arial",
      Font {
        size: 75.590_55,
        family: "Arial".to_owned(),
        ..Default::default()
      },
    ),
    (
      "20px sans-serif",
      Font {
        size: 20.0,
        family: "sans-serif".to_owned(),
        ..Default::default()
      },
    ),
    (
      "20px monospace",
      Font {
        size: 20.0,
        family: "monospace".to_owned(),
        ..Default::default()
      },
    ),
    (
      "50px Arial, sans-serif",
      Font {
        size: 50.0,
        family: "Arial,sans-serif".to_owned(),
        ..Default::default()
      },
    ),
    (
      "bold italic 50px Arial, sans-serif",
      Font {
        size: 50.0,
        weight: 700,
        style: FontStyle::Italic,
        family: "Arial,sans-serif".to_owned(),
        ..Default::default()
      },
    ),
    (
      "50px Helvetica ,  Arial, sans-serif",
      Font {
        size: 50.0,
        family: "Helvetica,Arial,sans-serif".to_owned(),
        ..Default::default()
      },
    ),
    (
      "50px \"Helvetica Neue\", sans-serif",
      Font {
        size: 50.0,
        family: "Helvetica Neue,sans-serif".to_owned(),
        ..Default::default()
      },
    ),
    (
      "100px 'Microsoft YaHei'",
      Font {
        size: 100.0,
        family: "Microsoft YaHei".to_owned(),
        ..Default::default()
      },
    ),
    (
      "300 20px Arial",
      Font {
        size: 20.0,
        weight: 300,
        family: "Arial".to_owned(),
        ..Default::default()
      },
    ),
    (
      "50px",
      Font {
        size: 50.0,
        family: "sans-serif".to_owned(),
        ..Default::default()
      },
    ),
    (
      "400 48px/57.599999999999994px Cascadia",
      Font {
        size: 48.0,
        weight: 400,
        family: "Cascadia".to_owned(),
        ..Default::default()
      },
    ),
  ];

  for (rule, expect) in fixtures.into_iter() {
    assert_eq!(Font::new(rule).unwrap(), expect);
  }
}
