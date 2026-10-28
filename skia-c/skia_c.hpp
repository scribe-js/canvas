#ifndef SKIA_CAPI_H
#define SKIA_CAPI_H

#include <include/codec/SkCodec.h>
#include <include/codec/SkEncodedImageFormat.h>
#include <include/core/SkAnnotation.h>
#include <include/core/SkBBHFactory.h>
#include <include/core/SkBitmap.h>
#include <include/core/SkBlurTypes.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkColorFilter.h>
#include <include/core/SkData.h>
#include <include/core/SkDrawable.h>
#include <include/core/SkFontMgr.h>
#include <include/core/SkGraphics.h>
#include <include/core/SkMaskFilter.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPathBuilder.h>
#include <include/core/SkPathEffect.h>
#include <include/core/SkPathUtils.h>
#include <include/core/SkPicture.h>
#include <include/core/SkPictureRecorder.h>
#include <include/core/SkSamplingOptions.h>
#include <include/core/SkStream.h>
#include <include/core/SkString.h>
#include <include/core/SkStrokeRec.h>
#include <include/core/SkSurface.h>
#include <include/docs/SkPDFDocument.h>
#include <include/docs/SkPDFJpegHelpers.h>
#include <include/effects/SkColorMatrix.h>
#include <include/effects/SkCornerPathEffect.h>
#include <include/effects/SkDashPathEffect.h>
#include <include/effects/SkGradient.h>
#include <include/effects/SkImageFilters.h>
#include <include/effects/SkTrimPathEffect.h>
#include <include/encode/SkJpegEncoder.h>
#include <include/encode/SkPngEncoder.h>
#include <include/encode/SkWebpEncoder.h>
#include <include/pathops/SkPathOps.h>
#include <include/svg/SkSVGCanvas.h>
#include <include/utils/SkParsePath.h>
#include <modules/skottie/include/Skottie.h>
#include <modules/skparagraph/include/FontCollection.h>
#include <modules/skparagraph/include/Paragraph.h>
#include <modules/skparagraph/include/ParagraphBuilder.h>
#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skparagraph/src/ParagraphImpl.h>
#include <modules/skresources/include/SkResources.h>
#include <modules/skunicode/include/SkUnicode_icu.h>
#include <modules/svg/include/SkSVGDOM.h>
#include <modules/svg/include/SkSVGNode.h>
#include <modules/svg/include/SkSVGRenderContext.h>
#include <modules/svg/include/SkSVGSVG.h>
#include <src/core/SkFontDescriptor.h>
#include <src/ports/SkFontMgr_custom.h>
#include <src/xml/SkXMLWriter.h>
#include <algorithm>
#include <cstring>
#include <map>
#include <iterator>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <stdint.h>

using namespace skia::textlayout;

typedef struct skiac_surface skiac_surface;
typedef struct skiac_canvas skiac_canvas;
typedef struct skiac_paint skiac_paint;
typedef struct skiac_path skiac_path;
typedef struct skiac_shader skiac_shader;
typedef struct skiac_path_effect skiac_path_effect;
typedef struct skiac_matrix skiac_matrix;
typedef struct skiac_mask_filter skiac_mask_filter;
typedef struct skiac_image_filter skiac_image_filter;
typedef struct skiac_data skiac_data;
typedef struct skiac_image skiac_image;
typedef struct skiac_bitmap skiac_bitmap;
typedef struct skiac_sk_string skiac_sk_string;
typedef struct skiac_font_metrics skiac_font_metrics;
typedef struct skiac_typeface skiac_typeface;
typedef struct skiac_font_mgr skiac_font_mgr;
typedef struct skiac_typeface_font_provider skiac_typeface_font_provider;
typedef struct skiac_w_memory_stream skiac_w_memory_stream;
typedef struct skiac_picture_recorder skiac_picture_recorder;
typedef struct skiac_picture skiac_picture;
typedef struct skiac_drawable skiac_drawable;
typedef struct skiac_encoder skiac_encoder;
typedef struct skiac_document skiac_document;
typedef struct skiac_skottie_animation skiac_skottie_animation;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define SK_FONT_FILE_PREFIX "C:/Windows/Fonts"
#elif __APPLE__
#define SK_FONT_FILE_PREFIX "/System/Library/Fonts/"
#elif __linux__
#define SK_FONT_FILE_PREFIX "/usr/share/fonts/"
#endif

sk_sp<SkFontMgr> SkFontMgr_New_Custom_Directory(const char* dir);

sk_sp<SkFontMgr> SkFontMgr_New_Custom_Empty();

enum class CssBaseline {
  Top,
  Hanging,
  Middle,
  Alphabetic,
  Ideographic,
  Bottom,
};

// Compute stable content-based ID from font data using FNV-1a hash
// IMPORTANT: Never returns 0 because Rust wrapper treats 0 as failure
inline uint32_t computeFontContentHash(const uint8_t* bytes, size_t size) {
  if (!bytes)
    return 1;  // Use 1 instead of 0 (0 means failure to Rust)
  // FNV-1a hash
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < size && i < 1024; i++) {  // Hash first 1KB for speed
    hash ^= bytes[i];
    hash *= 16777619u;
  }
  // Include size in hash to differentiate similar prefixes
  hash ^= static_cast<uint32_t>(size);
  return hash ? hash : 1;  // Never return 0
}

inline uint32_t computeFontContentHash(const sk_sp<SkData>& data) {
  if (!data)
    return 1;
  return computeFontContentHash(static_cast<const uint8_t*>(data->data()),
                                data->size());
}

// Compute stable content-based ID from file path using FNV-1a hash
// IMPORTANT: Never returns 0 because Rust wrapper treats 0 as failure
inline uint32_t computePathHash(const std::string& path) {
  if (path.empty())
    return 1;  // Use 1 for empty path (0 means failure to Rust)
  uint32_t hash = 2166136261u;
  for (char c : path) {
    hash ^= static_cast<uint8_t>(c);
    hash *= 16777619u;
  }
  return hash ? hash : 1;  // Never return 0
}

// Passkey accessor that exposes SkTypeface::setFontStyle (protected) so we
// can override a fresh typeface's reported SkFontStyle when the font file's
// OS/2.fsSelection / head.macStyle bits are wrong or missing. The setter is
// non-virtual and only writes to the POD member fStyle; fontStyle() reads it
// virtually via onGetFontStyle(). The FreeType-backed SkTypeface subclasses
// we produce via SkFontMgr_Custom do not override onGetFontStyle to do
// anything other than defer to the base member, so mutating fStyle before
// registration is sufficient for the style set's matching to see the
// override. Each makeFromData/makeFromFile call returns a unique SkTypeface,
// so the override reaches no other typeface.
struct SkTypefaceStyleAccess : public SkTypeface {
  using SkTypeface::setFontStyle;
};
inline void applyTypefaceStyleOverride(SkTypeface* typeface,
                                       const SkFontStyle& style) {
  static_cast<SkTypefaceStyleAccess*>(typeface)->setFontStyle(style);
}

// Immutable once built, since a set stays in use after the provider releases
// its lock.
class ScribeFontStyleSet : public SkFontStyleSet {
 public:
  struct Entry {
    sk_sp<SkTypeface> typeface;
    uint32_t count;
  };

  ScribeFontStyleSet(SkString family_name, std::vector<Entry> entries)
      : family_name(std::move(family_name)), entries(std::move(entries)) {}

  int count() override { return static_cast<int>(entries.size()); }

  void getStyle(int index, SkFontStyle* style, SkString* name) override {
    if (style) {
      *style = entries[index].typeface->fontStyle();
    }
    if (name) {
      *name = family_name;
    }
  }

  sk_sp<SkTypeface> createTypeface(int index) override {
    return entries[index].typeface;
  }

  sk_sp<SkTypeface> matchStyle(const SkFontStyle& pattern) override {
    return this->matchStyleCSS3(pattern);
  }

  const SkString family_name;
  const std::vector<Entry> entries;
};

class ScribeFontProvider : public SkFontMgr {
 public:
  void registerTypeface(sk_sp<SkTypeface> typeface,
                        const SkString& family_name) {
    if (!typeface || family_name.isEmpty()) {
      return;
    }
    std::unique_lock<std::shared_mutex> lock(mutex);
    std::string key(family_name.c_str());
    std::vector<ScribeFontStyleSet::Entry> entries;
    auto it = families.find(key);
    if (it != families.end()) {
      entries = it->second->entries;
    } else {
      family_names.push_back(key);
    }
    auto entry = std::find_if(entries.begin(), entries.end(),
                              [&](const ScribeFontStyleSet::Entry& e) {
                                return e.typeface.get() == typeface.get();
                              });
    if (entry != entries.end()) {
      entry->count++;
    } else {
      entries.push_back({std::move(typeface), 1});
    }
    families[key] =
        sk_make_sp<ScribeFontStyleSet>(family_name, std::move(entries));
  }

  void unregisterTypeface(const SkTypeface* typeface,
                          const SkString& family_name) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    auto it = families.find(std::string(family_name.c_str()));
    if (it == families.end()) {
      return;
    }
    std::vector<ScribeFontStyleSet::Entry> entries = it->second->entries;
    auto entry = std::find_if(entries.begin(), entries.end(),
                              [&](const ScribeFontStyleSet::Entry& e) {
                                return e.typeface.get() == typeface;
                              });
    if (entry == entries.end()) {
      return;
    }
    if (--entry->count == 0) {
      entries.erase(entry);
    }
    this->replaceFamily(it, std::move(entries));
  }

  // Drops `typeface` from every family whatever its count, since no
  // registration records the names setAlias gave it.
  void unregisterTypefaceEverywhere(const SkTypeface* typeface) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    for (auto it = families.begin(); it != families.end();) {
      std::vector<ScribeFontStyleSet::Entry> entries;
      for (const auto& entry : it->second->entries) {
        if (entry.typeface.get() != typeface) {
          entries.push_back(entry);
        }
      }
      if (entries.size() == it->second->entries.size()) {
        ++it;
      } else {
        it = this->replaceFamily(it, std::move(entries));
      }
    }
  }

  void clear() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    families.clear();
    family_names.clear();
  }

 protected:
  int onCountFamilies() const override {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return static_cast<int>(family_names.size());
  }

  void onGetFamilyName(int index, SkString* family_name) const override {
    std::shared_lock<std::shared_mutex> lock(mutex);
    family_name->set(family_names[index].c_str());
  }

  sk_sp<SkFontStyleSet> onCreateStyleSet(int index) const override {
    std::shared_lock<std::shared_mutex> lock(mutex);
    auto it = families.find(family_names[index]);
    return it == families.end() ? nullptr : it->second;
  }

  sk_sp<SkFontStyleSet> onMatchFamily(const char family_name[]) const override {
    return this->findFamily(family_name);
  }

  sk_sp<SkTypeface> onMatchFamilyStyle(
      const char family_name[],
      const SkFontStyle& pattern) const override {
    auto set = this->findFamily(family_name);
    return set ? set->matchStyle(pattern) : nullptr;
  }

  sk_sp<SkTypeface> onMatchFamilyStyleCharacter(const char[],
                                                const SkFontStyle&,
                                                const char*[],
                                                int,
                                                SkUnichar) const override {
    return nullptr;
  }

  sk_sp<SkTypeface> onMakeFromData(sk_sp<SkData>, int) const override {
    return nullptr;
  }

  sk_sp<SkTypeface> onMakeFromStreamIndex(std::unique_ptr<SkStreamAsset>,
                                          int) const override {
    return nullptr;
  }

  sk_sp<SkTypeface> onMakeFromStreamArgs(
      std::unique_ptr<SkStreamAsset>,
      const SkFontArguments&) const override {
    return nullptr;
  }

  sk_sp<SkTypeface> onMakeFromFile(const char[], int) const override {
    return nullptr;
  }

  // As skia's TypefaceFontProvider: the named family's match, else the first
  // family's.
  sk_sp<SkTypeface> onLegacyMakeTypeface(const char family_name[],
                                         SkFontStyle style) const override {
    if (family_name) {
      auto matched = this->matchFamilyStyle(family_name, style);
      if (matched) {
        return matched;
      }
    }
    sk_sp<ScribeFontStyleSet> first;
    {
      std::shared_lock<std::shared_mutex> lock(mutex);
      if (family_names.empty()) {
        return nullptr;
      }
      auto it = families.find(family_names.front());
      if (it == families.end()) {
        return nullptr;
      }
      first = it->second;
    }
    return first->matchStyle(style);
  }

 private:
  using Families =
      std::unordered_map<std::string, sk_sp<ScribeFontStyleSet>>;

  sk_sp<ScribeFontStyleSet> findFamily(const char* family_name) const {
    if (!family_name) {
      return nullptr;
    }
    std::shared_lock<std::shared_mutex> lock(mutex);
    auto it = families.find(std::string(family_name));
    return it == families.end() ? nullptr : it->second;
  }

  Families::iterator replaceFamily(
      Families::iterator it,
      std::vector<ScribeFontStyleSet::Entry> entries) {
    if (entries.empty()) {
      family_names.erase(
          std::find(family_names.begin(), family_names.end(), it->first));
      return families.erase(it);
    }
    it->second = sk_make_sp<ScribeFontStyleSet>(it->second->family_name,
                                                std::move(entries));
    return std::next(it);
  }

  mutable std::shared_mutex mutex;
  std::vector<std::string> family_names;
  Families families;
};

struct skiac_svg_surface {
  skiac_w_memory_stream* stream;
  skiac_surface* surface;
  skiac_canvas* canvas;
};

struct FontContent {
  sk_sp<SkData> data;
  std::string path;
  // The caller's style override is part of the content's identity, since it is
  // written into the typeface.
  int weight = -1;
  int width = -1;
  int slant = -1;
  sk_sp<SkTypeface> typeface;
  uint32_t registrants = 0;
};

struct FontRegistration {
  uint32_t content_id;
  std::vector<std::string> names;
};

struct skiac_font_collection {
  sk_sp<FontCollection> collection;
  sk_sp<SkFontMgr> font_mgr;
  sk_sp<ScribeFontProvider> assets;
  std::unordered_map<uint32_t, FontContent> contents;
  std::unordered_map<uint32_t, std::vector<uint32_t>> contents_by_hash;
  std::unordered_map<uint32_t, FontRegistration> registrations;
  uint32_t last_content_id = 0;
  uint32_t last_registration_id = 0;

  skiac_font_collection()
      : collection(sk_make_sp<FontCollection>()),
        font_mgr(SkFontMgr_New_Custom_Directory(SK_FONT_FILE_PREFIX)),
        assets(sk_make_sp<ScribeFontProvider>()) {
    collection->setDefaultFontManager(SkFontMgr_New_Custom_Empty());
    collection->setAssetFontManager(font_mgr);
    collection->setDynamicFontManager(assets);
    collection->enableFontFallback();
  }

  uint32_t registerFont(const uint8_t* bytes,
                        size_t length,
                        const char* path,
                        const char* alias,
                        int weight,
                        int width,
                        int slant) {
    uint32_t hash =
        bytes ? computeFontContentHash(bytes, length) : computePathHash(path);
    uint32_t content_id = 0;
    sk_sp<SkData> shared_data;
    auto bucket = contents_by_hash.find(hash);
    if (bucket != contents_by_hash.end()) {
      for (uint32_t id : bucket->second) {
        const FontContent& candidate = contents.find(id)->second;
        bool same_source =
            bytes ? (candidate.data && candidate.data->size() == length &&
                     memcmp(candidate.data->data(), bytes, length) == 0)
                  : (!candidate.path.empty() && candidate.path == path);
        if (!same_source) {
          continue;
        }
        shared_data = candidate.data;
        if (candidate.weight == weight && candidate.width == width &&
            candidate.slant == slant) {
          content_id = id;
          break;
        }
      }
    }
    if (content_id == 0) {
      FontContent fresh;
      if (bytes) {
        fresh.data =
            shared_data ? shared_data : SkData::MakeWithCopy(bytes, length);
        fresh.typeface = font_mgr->makeFromData(fresh.data);
      } else {
        fresh.path = path;
        fresh.typeface = font_mgr->makeFromFile(path);
      }
      if (!fresh.typeface) {
        return 0;
      }
      fresh.weight = weight;
      fresh.width = width;
      fresh.slant = slant;
      if (weight != -1 || width != -1 || slant != -1) {
        SkFontStyle base = fresh.typeface->fontStyle();
        applyTypefaceStyleOverride(
            fresh.typeface.get(),
            SkFontStyle(
                (weight >= 1 && weight <= 1000) ? weight : base.weight(),
                (width >= 1 && width <= 9) ? width : base.width(),
                (slant >= 0 && slant <= 2)
                    ? static_cast<SkFontStyle::Slant>(slant)
                    : base.slant()));
      }
      do {
        ++last_content_id;
      } while (last_content_id == 0 || contents.count(last_content_id));
      content_id = last_content_id;
      contents_by_hash[hash].push_back(content_id);
      contents[content_id] = std::move(fresh);
    }
    FontContent& content = contents.find(content_id)->second;
    content.registrants++;

    FontRegistration registration;
    registration.content_id = content_id;
    SkString family;
    content.typeface->getFamilyName(&family);
    if (!family.isEmpty()) {
      registration.names.push_back(family.c_str());
    }
    if (alias && *alias &&
        (family.isEmpty() || strcmp(alias, family.c_str()) != 0)) {
      registration.names.push_back(alias);
    }
    for (const auto& name : registration.names) {
      assets->registerTypeface(content.typeface, SkString(name.c_str()));
    }
    do {
      ++last_registration_id;
    } while (last_registration_id == 0 ||
             registrations.count(last_registration_id));
    registrations[last_registration_id] = std::move(registration);
    return last_registration_id;
  }

  // The caller must then purge the collection's caches, which may still resolve
  // the removed names.
  bool unregisterFont(uint32_t registration_id) {
    auto it = registrations.find(registration_id);
    if (it == registrations.end()) {
      return false;
    }
    FontContent& content = contents.find(it->second.content_id)->second;
    for (const auto& name : it->second.names) {
      assets->unregisterTypeface(content.typeface.get(),
                                 SkString(name.c_str()));
    }
    if (--content.registrants == 0) {
      assets->unregisterTypefaceEverywhere(content.typeface.get());
      uint32_t hash = content.data ? computeFontContentHash(content.data)
                                   : computePathHash(content.path);
      auto& ids = contents_by_hash.find(hash)->second;
      ids.erase(std::find(ids.begin(), ids.end(), it->second.content_id));
      if (ids.empty()) {
        contents_by_hash.erase(hash);
      }
      contents.erase(it->second.content_id);
    }
    registrations.erase(it);
    return true;
  }
};

typedef void (*write_callback_t)(const void* buffer,
                                 size_t size,
                                 void* context);

class SkJavaScriptWStream : public SkWStream {
 public:
  SkJavaScriptWStream(write_callback_t write_callback, void* context)
      : fBytesWritten(0), fWriteCallback(write_callback), fContext(context) {}

  bool write(const void* buffer, size_t size) override {
    fBytesWritten += size;
    fWriteCallback(buffer, size, fContext);
    return true;
  }

  void flush() override { fBytesWritten = 0; }

  size_t bytesWritten() const override { return fBytesWritten; }

 private:
  size_t fBytesWritten;
  write_callback_t fWriteCallback;
  void* fContext;
};
struct skiac_line_metrics {
  float ascent;
  float descent;
  float left;
  float right;
  float width;
  float font_ascent;
  float font_descent;
  float alphabetic_baseline;
};

struct skiac_rect {
  float left;
  float top;
  float right;
  float bottom;
};

struct skiac_transform {
  float a;
  float b;
  float c;
  float d;
  float e;
  float f;
};

struct skiac_point {
  float x;
  float y;
};

struct skiac_surface_data {
  uint8_t* ptr;
  size_t size;
};

struct skiac_bitmap_info {
  skiac_bitmap* bitmap;
  int width;
  int height;
  bool is_canvas;
};

struct skiac_string {
  const char* ptr;
  size_t length;
  SkString* sk_string;
};

typedef void (*skiac_on_match_font_style)(int width,
                                          int weight,
                                          int slant,
                                          void* skiac_on_match_font_style_rust);

struct skiac_sk_data {
  const uint8_t* ptr;
  size_t size;
  skiac_data* data;
};

struct skiac_mapped_point {
  float x;
  float y;
};

struct skiac_pdf_document {
  skiac_document* document;
  skiac_w_memory_stream* stream;
};

struct skiac_pdf_metadata {
  const char* title;
  const char* author;
  const char* subject;
  const char* keywords;
  const char* creator;
  const char* producer;
  float raster_dpi;
  int encoding_quality;
  bool pdfa;
  int compression_level;  // -1 = default, 0 = none, 1 = low, 6 = average, 9 =
                          // high
};

struct skiac_variable_font_axis {
  uint32_t tag;  // OpenType tag (e.g., 'wght', 'wdth', 'slnt', 'ital')
  float value;   // Current value for this axis
  float min;     // Minimum value for this axis
  float max;     // Maximum value for this axis
  float def;     // Default value for this axis
  bool hidden;   // Whether this axis should be hidden
};

struct skiac_font_variation {
  uint32_t tag;  // OpenType tag
  float value;   // Value for this axis
};

extern "C" {
void skiac_clear_all_cache();
// Surface
skiac_surface* skiac_surface_create_rgba_premultiplied(int width,
                                                       int height,
                                                       uint8_t cs);
void skiac_surface_create_svg(skiac_svg_surface* c_surface,
                              int width,
                              int height,
                              int alphaType,
                              uint32_t flag,
                              uint8_t cs);
skiac_surface* skiac_surface_create_rgba(int width, int height, uint8_t cs);
void skiac_surface_destroy(skiac_surface* c_surface);
skiac_surface* skiac_surface_copy_rgba(skiac_surface* c_surface,
                                       uint32_t x,
                                       uint32_t y,
                                       uint32_t width,
                                       uint32_t height,
                                       uint8_t cs);
skiac_canvas* skiac_surface_get_canvas(skiac_surface* c_surface);
int skiac_surface_get_width(skiac_surface* c_surface);
int skiac_surface_get_height(skiac_surface* c_surface);
void skiac_surface_read_pixels(skiac_surface* c_surface,
                               skiac_surface_data* data);
bool skiac_surface_read_pixels_rect(skiac_surface* c_surface,
                                    uint8_t* data,
                                    int x,
                                    int y,
                                    int w,
                                    int h,
                                    uint8_t cs);
void skiac_surface_png_data(skiac_surface* c_surface, skiac_sk_data* data);
void skiac_surface_encode_data(skiac_surface* c_surface,
                               skiac_sk_data* data,
                               int format,
                               int quality);
bool skiac_surface_encode_stream(skiac_surface* c_surface,
                                 int format,
                                 int quality,
                                 write_callback_t write_callback,
                                 void* context);
int skiac_surface_get_alpha_type(skiac_surface* c_surface);
bool skiac_surface_save(skiac_surface* c_surface, const char* path);
void skiac_surface_get_bitmap(skiac_surface* c_surface,
                              skiac_bitmap_info* info);

// Canvas
void skiac_canvas_clear(skiac_canvas* c_canvas, uint32_t color);
void skiac_canvas_set_transform(skiac_canvas* c_canvas, skiac_matrix* c_matrix);
void skiac_canvas_concat(skiac_canvas* c_canvas, skiac_matrix* c_matrix);
void skiac_canvas_scale(skiac_canvas* c_canvas, float sx, float sy);
void skiac_canvas_translate(skiac_canvas* c_canvas, float dx, float dy);
void skiac_canvas_rotate(skiac_canvas* c_canvas, float degrees);
skiac_transform skiac_canvas_get_total_transform(skiac_canvas* c_canvas);
skiac_matrix* skiac_canvas_get_total_transform_matrix(skiac_canvas* c_canvas);
void skiac_canvas_draw_color(skiac_canvas* c_canvas,
                             float r,
                             float g,
                             float b,
                             float a);
void skiac_canvas_draw_image(skiac_canvas* c_canvas,
                             skiac_bitmap* c_bitmap,
                             bool is_canvas,
                             float sx,
                             float sy,
                             float s_width,
                             float s_height,
                             float dx,
                             float dy,
                             float d_width,
                             float d_height,
                             bool enable_smoothing,
                             int filter_quality,
                             skiac_paint* c_paint);
void skiac_canvas_draw_path(skiac_canvas* c_canvas,
                            skiac_path* c_path,
                            skiac_paint* c_paint);
void skiac_canvas_draw_rect(skiac_canvas* c_canvas,
                            float x,
                            float y,
                            float w,
                            float h,
                            skiac_paint* c_paint);
void skiac_canvas_draw_surface(skiac_canvas* c_canvas,
                               skiac_surface* c_surface,
                               float left,
                               float top,
                               uint8_t alpha,
                               int blend_mode,
                               int filter_quality);
void skiac_canvas_draw_surface_rect(skiac_canvas* c_canvas,
                                    skiac_surface* c_surface,
                                    float sx,
                                    float sy,
                                    float sw,
                                    float sh,
                                    float dx,
                                    float dy,
                                    float dw,
                                    float dh,
                                    int filter_quality);
void skiac_canvas_get_line_metrics_or_draw_text(
    const char* text,
    size_t text_len,
    float max_width,
    float x,
    float y,
    float canvas_width,
    skiac_font_collection* c_collection,
    float font_size,
    int weight,
    int stretch,
    float stretch_width,
    int slant,
    const char* font_family,
    int baseline,
    int align,
    int direction,
    float letter_spacing,
    float world_spacing,
    skiac_paint* c_paint,
    skiac_canvas* c_canvas,
    skiac_line_metrics* c_line_metrics,
    const skiac_font_variation* variations,
    int variations_count,
    int kerning,
    int variant_caps,
    const char* lang,
    int text_rendering);
void skiac_canvas_reset_transform(skiac_canvas* c_canvas);
void skiac_canvas_clip_rect(skiac_canvas* c_canvas,
                            float x,
                            float y,
                            float w,
                            float h);
void skiac_canvas_clip_path(skiac_canvas* c_canvas, skiac_path* c_path);
void skiac_canvas_save(skiac_canvas* c_canvas);
void skiac_canvas_restore(skiac_canvas* c_canvas);
void skiac_canvas_reset(skiac_canvas* c_canvas);
void skiac_canvas_write_pixels(skiac_canvas* c_canvas,
                               int width,
                               int height,
                               uint8_t* pixels,
                               size_t row_bytes,
                               int x,
                               int y);
void skiac_canvas_put_image_data(skiac_canvas* c_canvas,
                                 int width,
                                 int height,
                                 uint8_t* pixels,
                                 size_t row_bytes,
                                 size_t length,
                                 float x,
                                 float y,
                                 float dirty_x,
                                 float dirty_y,
                                 float dirty_width,
                                 float dirty_height,
                                 uint8_t cs,
                                 bool snapshot);
void skiac_canvas_draw_picture(skiac_canvas* c_canvas,
                               skiac_picture* c_picture,
                               skiac_matrix* c_matrix,
                               skiac_paint* c_paint);
// Optimized: combines save/clip/transform/draw/restore into single call
void skiac_canvas_draw_picture_rect(skiac_canvas* c_canvas,
                                    skiac_picture* c_picture,
                                    float sx,
                                    float sy,
                                    float sw,
                                    float sh,
                                    float dx,
                                    float dy,
                                    float dw,
                                    float dh,
                                    skiac_paint* c_paint);
void skiac_canvas_destroy(skiac_canvas* c_canvas);

// Paint
skiac_paint* skiac_paint_create();
skiac_paint* skiac_paint_clone(skiac_paint* c_paint);
void skiac_paint_destroy(skiac_paint* c_paint);
void skiac_paint_set_style(skiac_paint* c_paint, int style);
void skiac_paint_set_color(skiac_paint* c_paint,
                           uint8_t r,
                           uint8_t g,
                           uint8_t b,
                           uint8_t a);
void skiac_paint_set_alpha(skiac_paint* c_paint, uint8_t a);
uint8_t skiac_paint_get_alpha(skiac_paint* c_paint);
void skiac_paint_set_anti_alias(skiac_paint* c_paint, bool aa);
void skiac_paint_set_blend_mode(skiac_paint* c_paint, int blend_mode);
int skiac_paint_get_blend_mode(skiac_paint* c_paint);
void skiac_paint_set_shader(skiac_paint* c_paint, skiac_shader* c_shader);
void skiac_paint_set_stroke_width(skiac_paint* c_paint, float width);
float skiac_paint_get_stroke_width(skiac_paint* c_paint);
void skiac_paint_set_stroke_cap(skiac_paint* c_paint, int cap);
int skiac_paint_get_stroke_cap(skiac_paint* c_paint);
void skiac_paint_set_stroke_join(skiac_paint* c_paint, uint8_t join);
uint8_t skiac_paint_get_stroke_join(skiac_paint* c_paint);
void skiac_paint_set_stroke_miter(skiac_paint* c_paint, float miter);
float skiac_paint_get_stroke_miter(skiac_paint* c_paint);
void skiac_paint_set_path_effect(skiac_paint* c_paint,
                                 skiac_path_effect* c_path_effect);
void skiac_paint_set_mask_filter(skiac_paint* c_paint,
                                 skiac_mask_filter* c_mask_filter);
void skiac_paint_set_image_filter(skiac_paint* c_paint,
                                  skiac_image_filter* c_image_filter);

// Path
skiac_path* skiac_path_create();
skiac_path* skiac_path_from_svg(char* svg_path);
skiac_path* skiac_path_clone(skiac_path* c_path);
void skiac_path_swap(skiac_path* c_path, skiac_path* other_path);
void skiac_add_path(skiac_path* c_path,
                    skiac_path* other_path,
                    skiac_matrix* c_matrix);
bool skiac_path_op(skiac_path* c_path_one, skiac_path* c_path_two, int op);
void skiac_path_to_svg_string(skiac_path* c_path, skiac_string* c_string);
bool skiac_path_simplify(skiac_path* c_path);
bool skiac_path_stroke(skiac_path* c_path,
                       int cap,
                       uint8_t join,
                       float width,
                       float miter_limit);
void skiac_path_get_bounds(skiac_path* c_path, skiac_rect* c_rect);
void skiac_path_compute_tight_bounds(skiac_path* c_path, skiac_rect* c_rect);
bool skiac_path_trim(skiac_path* c_path,
                     float start_t,
                     float stop_t,
                     bool is_complement);
bool skiac_path_dash(skiac_path* c_path, float on, float off, float phase);
bool skiac_path_round(skiac_path* c_path, float radius);
bool skiac_path_equals(skiac_path* c_path, skiac_path* other_path);
void skiac_path_destroy(skiac_path* c_path);
void skiac_path_set_fill_type(skiac_path* c_path, int type);
int skiac_path_get_fill_type(skiac_path* c_path);
bool skiac_path_as_winding(skiac_path* c_path);
void skiac_path_arc_to(skiac_path* c_path,
                       float left,
                       float top,
                       float right,
                       float bottom,
                       float startAngle,
                       float sweepAngle,
                       bool forceMoveTo);
void skiac_path_arc_to_tangent(skiac_path* c_path,
                               float x1,
                               float y1,
                               float x2,
                               float y2,
                               float radius);
void skiac_path_move_to(skiac_path* c_path, float x, float y);
void skiac_path_line_to(skiac_path* c_path, float x, float y);
void skiac_path_cubic_to(skiac_path* c_path,
                         float x1,
                         float y1,
                         float x2,
                         float y2,
                         float x3,
                         float y3);
void skiac_path_quad_to(skiac_path* c_path,
                        float cpx,
                        float cpy,
                        float x,
                        float y);
void skiac_path_close(skiac_path* c_path);
void skiac_path_add_rect(skiac_path* c_path,
                         float l,
                         float t,
                         float r,
                         float b);
void skiac_path_add_circle(skiac_path* c_path, float x, float y, float r);
skiac_path* skiac_path_transform(skiac_path* c_path, skiac_matrix* c_matrix);
void skiac_path_transform_self(skiac_path* c_path, skiac_matrix* c_matrix);
bool skiac_path_is_empty(skiac_path* c_path);
bool skiac_path_hit_test(skiac_path* c_path, float x, float y, int type);
bool skiac_path_stroke_hit_test(skiac_path* c_path,
                                float x,
                                float y,
                                float stroke_w);
void skiac_path_round_rect(skiac_path* c_path,
                           SkScalar x,
                           SkScalar y,
                           SkScalar width,
                           SkScalar height,
                           SkScalar* radii,
                           bool clockwise);

// PathEffect
skiac_path_effect* skiac_path_effect_make_dash_path(const float* intervals,
                                                    int count,
                                                    float phase);
void skiac_path_effect_destroy(skiac_path_effect* c_path_effect);

// Shader
skiac_shader* skiac_shader_make_linear_gradient(const skiac_point* points,
                                                const uint32_t* colors,
                                                const float* positions,
                                                int count,
                                                int tile_mode,
                                                uint32_t flags,
                                                skiac_transform c_ts);
skiac_shader* skiac_shader_make_radial_gradient(skiac_point start_point,
                                                float start_radius,
                                                skiac_point end_point,
                                                float end_radius,
                                                const uint32_t* colors,
                                                const float* positions,
                                                int count,
                                                int tile_mode,
                                                uint32_t flags,
                                                skiac_transform c_ts);
skiac_shader* skiac_shader_make_conic_gradient(float cx,
                                               float cy,
                                               float radius,
                                               const uint32_t* colors,
                                               const float* positions,
                                               int count,
                                               int tile_mode,
                                               uint32_t flags,
                                               skiac_transform c_ts);
skiac_shader* skiac_shader_make_from_surface_image(skiac_surface* c_surface,
                                                   skiac_transform c_ts,
                                                   int filter_quality);

void skiac_shader_ref(skiac_shader* c_shader);
void skiac_shader_destroy(skiac_shader* c_shader);

// Matrix
skiac_matrix* skiac_matrix_create();
skiac_matrix* skiac_matrix_new(float a,
                               float b,
                               float c,
                               float d,
                               float e,
                               float f);
skiac_matrix* skiac_matrix_from_ts(const skiac_transform* c_ts);
skiac_matrix* skiac_matrix_create_rotated(float rotation, float x, float y);
skiac_matrix* skiac_matrix_create_translated(float x, float y);
skiac_matrix* skiac_matrix_concat(skiac_matrix* c_matrix, skiac_matrix* other);
skiac_matrix* skiac_matrix_multiply(skiac_matrix* c_matrix,
                                    skiac_matrix* other);
skiac_matrix* skiac_matrix_clone(skiac_matrix* c_matrix);
void skiac_matrix_map_points_1(skiac_matrix* c_matrix,
                               float x,
                               float y,
                               skiac_mapped_point* mapped_point);
void skiac_matrix_pre_translate(skiac_matrix* c_matrix, float dx, float dy);
void skiac_matrix_pre_concat(skiac_matrix* c_matrix, skiac_matrix* other);
void skiac_matrix_pre_scale(skiac_matrix* c_matrix, float sx, float sy);
void skiac_matrix_pre_concat_transform(skiac_matrix* c_matrix,
                                       skiac_transform c_ts);
void skiac_matrix_pre_rotate(skiac_matrix* c_matrix, float degrees);
void skiac_matrix_pre_rotate_x_y(skiac_matrix* c_matrix,
                                 float degrees,
                                 float x,
                                 float y);
bool skiac_matrix_invert(skiac_matrix* c_matrix, skiac_matrix* inverse);
skiac_transform skiac_matrix_to_transform(skiac_matrix* c_matrix);
void skiac_matrix_destroy(skiac_matrix* c_matrix);

// MaskFilter
skiac_mask_filter* skiac_mask_filter_make_blur(float radius);
void skiac_mask_filter_destroy(skiac_mask_filter* c_mask_filter);

// ImageFilter
skiac_image_filter* skiac_image_filter_make_drop_shadow_only(
    float dx,
    float dy,
    float sigma_x,
    float sigma_y,
    uint32_t color,
    skiac_image_filter* c_image_filter);
skiac_image_filter* skiac_image_filter_make_drop_shadow(
    float dx,
    float dy,
    float sigma_x,
    float sigma_y,
    uint32_t color,
    skiac_image_filter* c_image_filter);
skiac_image_filter* skiac_image_filter_make_blur(
    float sigma_x,
    float sigma_y,
    skiac_image_filter* c_image_filter);
skiac_image_filter* skiac_image_filter_color_filter(
    float m00,
    float m01,
    float m02,
    float m10,
    float m11,
    float m12,
    float m20,
    float m21,
    float m22,
    float opacity,
    skiac_image_filter* c_image_filter);
skiac_image_filter* skiac_image_filter_from_argb(
    const uint8_t table_a[256],
    const uint8_t table_r[256],
    const uint8_t table_g[256],
    const uint8_t table_b[256],
    skiac_image_filter* c_image_filter);
void skiac_image_filter_destroy(skiac_image_filter* c_image_filter);

// SkImage (for PageCache)
skiac_image* skiac_surface_make_image_snapshot(skiac_surface* c_surface);
void skiac_image_ref(skiac_image* c_image);
void skiac_image_destroy(skiac_image* c_image);
int skiac_image_get_width(skiac_image* c_image);
int skiac_image_get_height(skiac_image* c_image);
void skiac_canvas_draw_sk_image(skiac_canvas* c_canvas,
                                skiac_image* c_image,
                                float left,
                                float top,
                                int filter_quality);

// Data
void skiac_sk_data_destroy(skiac_data* c_data);

// Bitmap
void skiac_bitmap_make_from_buffer(const uint8_t* ptr,
                                   size_t size,
                                   skiac_bitmap_info* bitmap_info);
bool skiac_bitmap_make_from_svg(const uint8_t* data,
                                size_t length,
                                float width,
                                float height,
                                skiac_bitmap_info* bitmap_info,
                                skiac_font_collection* c_collection,
                                uint8_t cs);
skiac_bitmap* skiac_bitmap_make_from_image_data(uint8_t* ptr,
                                                size_t width,
                                                size_t height,
                                                size_t row_bytes,
                                                size_t size,
                                                int ct,
                                                int at);
size_t skiac_bitmap_get_width(skiac_bitmap* c_bitmap);
size_t skiac_bitmap_get_height(skiac_bitmap* c_bitmap);
skiac_shader* skiac_bitmap_get_shader(
    bool is_canvas,
    skiac_bitmap* c_bitmap,
    int repeat_x,
    int repeat_y,
    float B,
    float C,  // See SkSamplingOptions.h for docs.
    skiac_transform c_ts);
void skiac_bitmap_destroy(skiac_bitmap* c_bitmap);

// SkString
void skiac_delete_sk_string(skiac_sk_string* c_sk_string);

// FontCollection
skiac_font_collection* skiac_font_collection_create();
uint32_t skiac_font_collection_get_default_fonts_count(
    skiac_font_collection* c_font_collection);
void skiac_font_collection_get_family(
    skiac_font_collection* c_font_collection,
    uint32_t i,
    skiac_string* c_string,
    void* on_get_style_rust,
    skiac_on_match_font_style on_match_font_style);
uint32_t skiac_font_collection_register(
    skiac_font_collection* c_font_collection,
    const uint8_t* font,
    size_t length,
    const char* name_alias);
uint32_t skiac_font_collection_register_from_path(
    skiac_font_collection* c_font_collection,
    const char* font_path,
    const char* name_alias);
size_t skiac_font_collection_unregister(
    skiac_font_collection* c_font_collection,
    uint32_t typeface_id);
size_t skiac_font_collection_unregister_batch(
    skiac_font_collection* c_font_collection,
    const uint32_t* typeface_ids,
    size_t count);
size_t skiac_font_collection_unregister_all(
    skiac_font_collection* c_font_collection);
bool skiac_font_collection_set_alias(skiac_font_collection* c_font_collection,
                                     const char* family,
                                     const char* alias);
void skiac_font_collection_destroy(skiac_font_collection* c_font_collection);

// Variable Fonts
int skiac_typeface_get_variation_design_position(
    skiac_font_collection* c_font_collection,
    const char* family_name,
    int weight,
    int width,
    int slant,
    skiac_variable_font_axis* axes,
    int max_axis_count);
bool skiac_font_has_variations(skiac_font_collection* c_font_collection,
                               const char* family_name,
                               int weight,
                               int width,
                               int slant);

// SkDynamicMemoryWStream
void skiac_sk_w_stream_get(skiac_w_memory_stream* c_w_memory_stream,
                           skiac_sk_data* sk_data,
                           int width,
                           int height);
void skiac_sk_w_stream_destroy(skiac_w_memory_stream* c_w_memory_stream);

// SkSVG
void skiac_svg_text_to_path(const uint8_t* data,
                            size_t length,
                            skiac_font_collection* c_collection,
                            skiac_sk_data* output_data);

// SkPictureRecorder
void skiac_picture_ref(skiac_picture* c_picture);
void skiac_picture_destroy(skiac_picture* c_picture);
void skiac_picture_playback(skiac_picture* c_picture, skiac_canvas* c_canvas);
skiac_picture_recorder* skiac_picture_recorder_create();
void skiac_picture_recorder_destroy(skiac_picture_recorder* c_picture_recorder);
void skiac_picture_recorder_begin_recording(
    skiac_picture_recorder* c_picture_recorder,
    float x,
    float y,
    float width,
    float height,
    bool use_bbh);
skiac_canvas* skiac_picture_recorder_get_recording_canvas(
    skiac_picture_recorder* c_picture_recorder);
skiac_picture* skiac_picture_recorder_finish_recording_as_picture(
    skiac_picture_recorder* c_picture_recorder);
skiac_drawable* skiac_picture_recorder_finish_recording_as_drawable(
    skiac_picture_recorder* c_picture_recorder);

// SkDrawable
void skiac_canvas_draw_drawable(skiac_canvas* c_canvas,
                                skiac_drawable* c_drawable,
                                skiac_matrix* c_matrix);
void skiac_drawable_destroy(skiac_drawable* c_drawable);

// SkDocument
void skiac_document_create(skiac_pdf_document* c_document,
                           const skiac_pdf_metadata* metadata = nullptr);
void skiac_document_destroy(skiac_pdf_document* c_document);
skiac_canvas* skiac_document_begin_page(skiac_pdf_document* c_document,
                                        float width,
                                        float height,
                                        skiac_rect* content = nullptr);
void skiac_document_end_page(skiac_pdf_document* c_document);
void skiac_document_close(skiac_pdf_document* c_document,
                          skiac_sk_data* output_data);

// SkAnnotation
void skiac_canvas_annotate_link_url(skiac_canvas* c_canvas,
                                    const skiac_rect* rect,
                                    const char* url);
void skiac_canvas_annotate_named_destination(skiac_canvas* c_canvas,
                                             float x,
                                             float y,
                                             const char* name);
void skiac_canvas_annotate_link_to_destination(skiac_canvas* c_canvas,
                                               const skiac_rect* rect,
                                               const char* name);

// Skottie (Lottie Animation)
skiac_skottie_animation* skiac_skottie_animation_make(
    const char* data,
    size_t length,
    const char* resource_path);
skiac_skottie_animation* skiac_skottie_animation_make_from_file(
    const char* path);
void skiac_skottie_animation_destroy(skiac_skottie_animation* c_animation);
double skiac_skottie_animation_get_duration(
    skiac_skottie_animation* c_animation);
double skiac_skottie_animation_get_fps(skiac_skottie_animation* c_animation);
double skiac_skottie_animation_get_in_point(
    skiac_skottie_animation* c_animation);
double skiac_skottie_animation_get_out_point(
    skiac_skottie_animation* c_animation);
void skiac_skottie_animation_get_size(skiac_skottie_animation* c_animation,
                                      float* width,
                                      float* height);
void skiac_skottie_animation_get_version(skiac_skottie_animation* c_animation,
                                         skiac_string* c_string);
void skiac_skottie_animation_seek(skiac_skottie_animation* c_animation,
                                  float t);
void skiac_skottie_animation_seek_frame(skiac_skottie_animation* c_animation,
                                        double frame);
void skiac_skottie_animation_seek_frame_time(
    skiac_skottie_animation* c_animation,
    double t);
void skiac_skottie_animation_render(skiac_skottie_animation* c_animation,
                                    skiac_canvas* c_canvas,
                                    const skiac_rect* dst);
void skiac_skottie_animation_render_with_flags(
    skiac_skottie_animation* c_animation,
    skiac_canvas* c_canvas,
    const skiac_rect* dst,
    uint32_t flags);
}

#endif  // SKIA_CAPI_H
