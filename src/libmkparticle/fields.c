#include "libmkparticle/config.h"
#include "libmkparticle/fields.h"
#include "libmkparticle/vm.h"
#include "runtime/cstring.h"

void* pfx_get_field(void* pfx, int index, int type);

PfxFieldDefinition properties[] = {
    {0x300, 0x001, 1},
    {0x303, 0x004, 1},
    {0x304, 0x008, 1},
    {0x301, 0x002, 3},
    {0x302, 0x200, 4},
    {0x305, 0x010, 3},
    {0x306, 0x020, 3},
    {0x307, 0x040, 4},
    {0x308, 0x080, 4},
    {0x309, 0x100, 5},
    {0, 0, 0},
};

PfxFieldDefinition render_fields[] = {
    {0x100, 0x002, 1},
    {0x104, 0x100, 1},
    {0x101, 0x010, 2},
    {0x102, 0x020, 3},
    {0x103, 0x040, 3},
    {0x163, 0x200, 1},
};

PfxFieldDefinition parametric_fields[] = {
    {0x400, 0, 1},
    {0x402, 0, 3},
    {0x403, 0, 3},
    {0, 0, 0},
};

const int _num_render_fields = 6;

int get_size(int type)
{
    switch (type) {
    case 1:
        return 12;
    case 2:
        return 4;
    case 4:
        return 4;
    case 5:
        return 8;
    case 3:
        return 4;
    default:
        return 0;
    }
}

unsigned int map_field_to_propertyflag(int description)
{
    int index;

    index = 0;
    while ((int)properties[index].flag != 0) {
        if (description == (int)properties[index].description) {
            return properties[index].flag;
        }
        index++;
    }
    return 0;
}

unsigned int map_field_to_renderflag(int description)
{
    int index;

    index = 0;
    while ((int)render_fields[index].flag != 0) {
        if (description == (int)render_fields[index].description) {
            return render_fields[index].flag;
        }
        index++;
    }
    return 0;
}

void add_field(unsigned int* fields, unsigned int description)
{
    unsigned int flag;
    unsigned int* destination;

    switch (description & 0xF00) {
    case 0x100:
    case 0x400:
        description = (description & ~0xF00) | 0x100;
        flag = map_field_to_renderflag(description);
        destination = fields;
        break;
    case 0x300:
        flag = map_field_to_propertyflag(description);
        destination = fields + 1;
        break;
    default:
        return;
    }
    *destination |= flag;
}

int map_field_to_stream(int description)
{
    switch (description & 0xF00) {
    case 0x100:
        return 0;
    case 0x400:
        return 2;
    case 0x300:
        return 1;
    case 0x200:
        return 3;
    default:
        return -1;
    }
}

int get_field_offset(PfxTableRegistry* registry, int description)
{
    unsigned char* field_base;
    unsigned char* data;
    int field_description;
    int index;

    field_description = description;
    if ((field_description & 0xF00) == 0x200) {
        field_base = registry->field_0x1FC;
        data = (unsigned char*)pfx_get_field(registry, 0,
                                             field_description);
        if (data != 0) {
            return data - field_base;
        }
        return 0;
    }
    if ((field_description & 0xF00) == 0x500) {
        return 0;
    }

    for (index = 0; index < registry->field_count; index++) {
        if (field_description ==
            registry->field_descriptions[index].description) {
            return registry->field_descriptions[index].offset;
        }
    }
    return 0;
}

static int count_bits(unsigned int flags)
{
    int bit;
    int count;

    count = 0;
    for (bit = 0; bit < 32; bit++) {
        if ((flags & (1U << bit)) != 0) {
            count++;
        }
    }
    return count;
}

int get_field_count(PfxFieldSet* fields)
{
    int particle_count;
    int render_count;

    particle_count = count_bits(fields->particle_flags);
    render_count = count_bits(fields->render_flags);
    render_count += particle_count;
    return render_count;
}

int has_field_description(PfxTableRegistry* registry, int description)
{
    int index;

    for (index = 0; index < registry->field_count; index++) {
        if (description == registry->field_descriptions[index].description) {
            return 1;
        }
    }
    return 0;
}

static void copy_field_v3(PfxFieldBuffer* destination,
                          PfxFieldBuffer* source, int count)
{
    unsigned char* source_data;
    unsigned char* destination_data;

    source_data = source->data;
    destination_data = destination->data;
    while (count-- > 0) {
        *(PfxVec3*)destination_data = *(PfxVec3*)source_data;
        destination_data += destination->stride;
        source_data += source->stride;
    }
}

void field_copy(PfxFieldBuffer* destination, PfxFieldBuffer* source,
                unsigned int field_size, int count)
{
    unsigned char* destination_data;
    unsigned char* source_data;
    int index;

    if (source->data == destination->data) {
        return;
    }
    if (field_size == sizeof(PfxVec3)) {
        copy_field_v3(destination, source, count);
        return;
    }

    destination_data = destination->data;
    source_data = source->data;
    for (index = 0; index < count; index++) {
        memcpy(destination_data, source_data, field_size);
        destination_data += destination->stride;
        source_data += source->stride;
    }
}

static void add_field_description(PfxFieldDescription* field,
                                  int description, int stream, int offset)
{
    field->description = description;
    field->stream = stream;
    field->offset = offset;
}

void fill_field_description(PfxFieldDescription* descriptions,
                            PfxFieldSet* fields, int parametric)
{
    int render_offset;
    PfxFieldDescription* description;
    int property_index;
    int property_offset;

    render_offset = 0;
    description = descriptions;
    if ((fields->render_flags & 0x002) != 0) {
        add_field_description(description, 0x100, 0, 0);
        render_offset =
            (get_size(1) + _pfx_config.align_add) & ~_pfx_config.align_mask;
        description++;
    }
    if ((fields->render_flags & 0x040) != 0) {
        add_field_description(description, 0x103, 0, render_offset);
        render_offset = (render_offset + get_size(3) +
                         _pfx_config.align_add) & ~_pfx_config.align_mask;
        description++;
    }
    if ((fields->render_flags & 0x010) != 0) {
        add_field_description(description, 0x101, 0, render_offset);
        render_offset = (render_offset + get_size(2) +
                         _pfx_config.align_add) & ~_pfx_config.align_mask;
        description++;
    }
    if ((fields->render_flags & 0x020) != 0) {
        add_field_description(description, 0x102, 0, render_offset);
        render_offset = (render_offset + get_size(3) +
                         _pfx_config.align_add) & ~_pfx_config.align_mask;
        description++;
    }
    if ((fields->render_flags & 0x100) != 0) {
        add_field_description(description, 0x104, 0, render_offset);
        render_offset = (render_offset + 0x10 + _pfx_config.align_add) &
                        ~_pfx_config.align_mask;
        description++;
    }
    if ((fields->render_flags & 0x200) != 0 &&
        _pfx_config.estimate_size != 0 && parametric == 0) {
        add_field_description(description, 0x163, 0, render_offset);
        description++;
    }

    property_offset = 0;
    property_index = 0;
    while ((int)properties[property_index].flag != 0) {
        if ((fields->particle_flags & properties[property_index].flag) != 0) {
            add_field_description(description,
                                  properties[property_index].description, 1,
                                  property_offset);
            property_offset += get_size(properties[property_index].type);
            description++;
        }
        property_index++;
    }

    if (parametric != 0) {
        add_field_description(description, 0x400, 2, 0);
        add_field_description(description + 1, 0x300, 2, 0xC);
        add_field_description(description + 2, 0x403, 2, 0x1C);
        add_field_description(description + 3, 0x402, 2, 0x20);
        description += 4;
    }
    add_field_description(description, -1, -1, -1);
}
