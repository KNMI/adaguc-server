#include <cstdint>
#include <vector>
#include "CDebugger.h"
#include "CStopWatch.h"
#include "CDrawImage.h"
#include "write_webp.h"

#include "webp/encode.h"
#include "webp/types.h"

static int webPWriter(const uint8_t *data, size_t data_size, const WebPPicture *const pic) {
  FILE *const out = (FILE *)pic->custom_ptr;
  return data_size ? (fwrite(data, data_size, 1, out) == 1) : 1;
}

int writeWebP(int width, int height, const unsigned char *ARGBByteBuffer, FILE *file, int quality) {
  // https://developers.google.com/speed/webp/docs/api
  const int stride = width * 4;

  WebPConfig config;
  WebPPicture picture;
  if (!WebPPictureInit(&picture) || !WebPConfigInit(&config)) {
    CDBError("Error! WEBP Version mismatch!\n");
    return 1;
  }

  config.lossless = quality == 100 ? 1 : 0; // Lossless encoding (0=lossy(default), 1=lossless).
  config.quality = quality;                 // between 0 and 100. For lossy, 0 gives the smallest
  config.method = 0;                        // quality/speed trade-off (0=fast, 6=slower-better)
  config.alpha_quality = 100;               // Between 0 (smallest size) and 100 (lossless). Default is 100.
  config.pass = 1;                          // number of entropy-analysis passes (in [1..10]).
  config.preprocessing = 0;                 // preprocessing filter (0=none, 1=segment-smooth)
  config.partitions = 0;                    // log2(number of token partitions) in [0..3] Default is set to 0 for easier progressive decoding.
  config.partition_limit = 100;             // quality degradation allowed to fit the 512k limit on prediction modes coding (0: no degradation, 100: maximum possible degradation).
  picture.use_argb = config.lossless ? 1 : 0; // To select between ARGB and YUVA input.
  config.thread_level = 1; // ~20-30% faster encoding, same output size (benchmarked on 1200x1000-3600x3000 lossy encodes).
  picture.width = width;
  picture.height = height;
  picture.writer = webPWriter;
  picture.custom_ptr = (void *)file;

  if (!WebPValidateConfig(&config)) {
    CDBError("Error! Invalid configuration.");
    return 1;
  }

  StopWatch_Measure("writeWebP: start import %dx%d", width, height);
  // libwebp wants straight alpha; only unpremultiply (into a scratch copy) if any pixel needs it.
  auto hasPartialAlpha = [&]() {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        unsigned char a = ARGBByteBuffer[x * 4 + y * stride + 3];
        if (a != 255 && a != 0) return true;
      }
    }
    return false;
  };
  std::vector<unsigned char> straight;
  const unsigned char *importSource = ARGBByteBuffer;
  if (hasPartialAlpha()) {
    straight.assign(importSource, importSource + size_t(height) * stride);
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        unsigned char *px = &straight[x * 4 + y * stride]; // B, G, R, A
        unsigned char a = px[3];
        if (a != 255 && a != 0) {
          px[0] = CDrawImage::unpremultiplyChannel(px[0], a);
          px[1] = CDrawImage::unpremultiplyChannel(px[1], a);
          px[2] = CDrawImage::unpremultiplyChannel(px[2], a);
        }
      }
    }
    importSource = straight.data();
  }
  bool importOk = WebPPictureImportBGRA(&picture, importSource, stride);
  if (!importOk) {
    CDBError("Error! Cannot import picture for WebP");
    WebPPictureFree(&picture);
    return 1;
  }
  StopWatch_Measure("writeWebP: done import, start encode");

  int status = 0;
  if (!WebPEncode(&config, &picture)) {
    CDBError("Error!  Cannot encode picture as WebP");
    status = 1;
  }
  StopWatch_Measure("writeWebP: done encode");
  WebPPictureFree(&picture);
  return status;
}
