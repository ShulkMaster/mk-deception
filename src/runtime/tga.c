#include "runtime/tga.h"

#include "runtime/mk_hwfile.h"
#include "runtime/mk_mem.h"

struct TgaHeader {
  unsigned char id_length;
  unsigned char color_map_type;
  unsigned char image_type;
  unsigned char color_map_first_lo;
  unsigned char color_map_first_hi;
  unsigned char color_map_length_lo;
  unsigned char color_map_length_hi;
  unsigned char color_map_depth;
  unsigned char origin_x_lo;
  unsigned char origin_x_hi;
  unsigned char origin_y_lo;
  unsigned char origin_y_hi;
  unsigned char width_lo;
  unsigned char width_hi;
  unsigned char height_lo;
  unsigned char height_hi;
  unsigned char pixel_depth;
  unsigned char descriptor;
};

struct TgaHeaderValues {
  int id_length;
  int color_map_type;
  int image_type;
  int color_map_first;
  int color_map_length;
  int color_map_depth;
  int origin_x;
  unsigned int origin_y;
  int width;
  int height;
  int pixel_depth;
  int descriptor;
};

typedef char TgaHeaderSizeCheck[sizeof(struct TgaHeader) == 0x12 ? 1 : -1];
typedef char TgaHeaderValuesSizeCheck[
    sizeof(struct TgaHeaderValues) == 0x30 ? 1 : -1];

static inline RwImage *tga_write_pixels(MkHwFileRequest *file, RwImage *image,
                                        struct TgaHeaderValues values) {
  int block_bytes;
  int row_bytes;
  unsigned char *pixels;
  int row;
  unsigned char *output;
  int width;
  int rows;
  int column;

  output = get_mem(0x1e00);
  if (output == 0) {
    return 0;
  }
  width = values.width;
  pixels = image->pixels;
  row_bytes = width * 3;
  row = values.height;
  block_bytes = width * 12;
  while (row > 0) {
    unsigned char *destination = output;

    rows = 0;
    do {
      row--;
      for (column = 0; column < values.width; column++) {
        destination[column * 3] = pixels[(column + row * values.width) * 4 + 2];
        destination[column * 3 + 1] = pixels[(column + row * values.width) * 4 + 1];
        destination[column * 3 + 2] = pixels[(column + row * values.width) * 4];
      }
      rows++;
      destination += row_bytes;
    } while (rows < 4);
    debug_file_write(file, output, block_bytes);
  }
  return image;
}

/* TODO: [near miss] 99.02439%; eligible LI24 loses to origin-Y extraction rank;
 * missing header-init producer must preserve the frame and sole value copy. */
RwImage *ImageWriteTGA(RwImage *image, const char *path) {
  RwImage *result;
  MkHwFileRequest *file;
  struct TgaHeader header;
  struct TgaHeaderValues values;

  file = debug_file_open(path, "w");
  if (file != 0) {
    values.id_length = 0;
    values.color_map_type = 0;
    values.image_type = 2;
    values.color_map_first = 0;
    values.color_map_length = 0;
    values.color_map_depth = 0;
    values.origin_x = 0;
    values.origin_y = 0;
    values.width = (unsigned short)image->width;
    values.height = (unsigned short)image->height;
    values.pixel_depth = 24;
    values.descriptor = 0;

    header.id_length = values.id_length;
    header.color_map_type = values.color_map_type;
    header.image_type = values.image_type;
    header.color_map_first_lo = values.color_map_first;
    header.color_map_first_hi =
        ((values.color_map_first & 0xff00) >> 8);
    header.color_map_length_lo = values.color_map_length;
    header.color_map_length_hi =
        ((values.color_map_length & 0xff00) >> 8);
    header.color_map_depth = values.color_map_depth;
    header.origin_x_lo = values.origin_x;
    header.origin_x_hi = ((values.origin_x & 0xff00) >> 8);
    header.origin_y_lo = values.origin_y;
    header.origin_y_hi = (values.origin_y >> 8);
    header.width_lo = values.width;
    header.width_hi = (values.width >> 8);
    header.height_lo = values.height;
    header.height_hi = (values.height >> 8);
    header.pixel_depth = values.pixel_depth;
    header.descriptor = values.descriptor;
    debug_file_write(file, &header, sizeof(header));

    result = tga_write_pixels(file, image, values);
    debug_file_close(file);
    return result;
  }
  return 0;
}
