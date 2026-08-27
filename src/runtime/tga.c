#include "runtime/tga.h"

#include "runtime/mk_hwfile.h"
#include "runtime/mk_mem.h"

typedef struct TgaHeader {
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
} TgaHeader;

typedef struct TgaHeaderValues {
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
} TgaHeaderValues;

/*
 * Retail builds the packed 18-byte header from this word-sized description.
 * Keeping both forms also preserves its unsigned 16-bit width/height clamp.
 * Soft ceiling: ImageWriteTGA is 96.20%; the remaining differences are
 * register allocation plus scheduling of one equivalent header-byte extract.
 */

extern MkHwFileRequest *debug_file_open(const char *path, const char *mode);
extern int debug_file_write(MkHwFileRequest *file, void *buffer, int length);
extern void debug_file_close(MkHwFileRequest *file);

/* Retail owns these bytes here, in this order, with the word at .rodata+4. */
__declspec(section ".rodata") static const char tga_write_mode[] = "w";
__declspec(section ".rodata") const int gap_04_8030366C_rodata = 0;

RwImage *ImageWriteTGA(RwImage *image, const char *path) {
  MkHwFileRequest *file;
  TgaHeader header;
  TgaHeaderValues values;
  TgaHeaderValues output_values;
  unsigned char *output;
  unsigned char *pixels;
  RwImage *result;
  int row;
  int rows;

  file = debug_file_open(path, tga_write_mode);
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

    header.id_length = (unsigned char)values.id_length;
    header.color_map_type = (unsigned char)values.color_map_type;
    header.image_type = (unsigned char)values.image_type;
    header.color_map_first_lo = (unsigned char)values.color_map_first;
    header.color_map_first_hi =
        (unsigned char)((values.color_map_first & 0xff00) >> 8);
    header.color_map_length_lo = (unsigned char)values.color_map_length;
    header.color_map_length_hi =
        (unsigned char)((values.color_map_length & 0xff00) >> 8);
    header.color_map_depth = (unsigned char)values.color_map_depth;
    header.origin_x_lo = (unsigned char)values.origin_x;
    header.origin_x_hi = (unsigned char)((values.origin_x & 0xff00) >> 8);
    header.origin_y_lo = (unsigned char)values.origin_y;
    header.origin_y_hi = (unsigned char)(values.origin_y >> 8);
    header.width_lo = (unsigned char)values.width;
    header.width_hi = (unsigned char)(values.width >> 8);
    header.height_lo = (unsigned char)values.height;
    header.height_hi = (unsigned char)(values.height >> 8);
    header.pixel_depth = (unsigned char)values.pixel_depth;
    header.descriptor = (unsigned char)values.descriptor;
    debug_file_write(file, &header, sizeof(header));

    output_values = values;
    output = get_mem(0x1e00);
    if (output == 0) {
      result = 0;
    } else {
      int row_bytes = output_values.width * 3;

      pixels = image->pixels;
      row = output_values.height;
      while (row > 0) {
        unsigned char *destination = output;

        rows = 0;
        do {
          int column;
          int output_offset;

          row--;
          output_offset = 0;
          for (column = 0; column < output_values.width; column++) {
            unsigned char *source =
                pixels + (column + row * output_values.width) * 4;
            destination[output_offset] = source[2];
            destination[output_offset + 1] = source[1];
            destination[output_offset + 2] = source[0];
            output_offset += 3;
          }
          rows++;
          destination += row_bytes;
        } while (rows < 4);
        debug_file_write(file, output, output_values.width * 12);
      }
      result = image;
    }

    debug_file_close(file);
    return result;
  }
  return 0;
}
