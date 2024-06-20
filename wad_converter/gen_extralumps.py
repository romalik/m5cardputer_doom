
from os import listdir, stat
from os.path import isfile, join

extra_lumps_folder = "extralumps"

lump_files = [f for f in listdir(extra_lumps_folder) if isfile(join(extra_lumps_folder, f))]

print(lump_files)

with open("extra_lumps_gen.h", "w") as hfile:
    hfile.write("#define EXTRA_LUMPS_N " + str(len(lump_files)) + "\n")


def bname(fname):
    return fname.split('.')[0]


"""
#include "extra_lumps.h"

char extra_1[] = {0x00, 0x01, 0x02, 0x03, 0x04};
char extra_2[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16};


struct extra_lump extra_lumps[EXTRA_LUMPS_N] = {
    {"EXTRLMP1", 5, extra_1},
    {"EXTRLMP2", 7, extra_2}
};
"""

def bytes_to_c_arr(data, lowercase=True):
    return [format(b, '#04x' if lowercase else '#04X') for b in data]



with open("extra_lumps.c", "w") as cfile:
    cfile.write('#include "extra_lumps.h"\n')


    for lump in lump_files:
        cfile.write('static char ' + bname(lump) + '[] = {')
        with open(join(extra_lumps_folder, lump), 'rb') as f:
            hexdata = f.read()
            bytes = bytes_to_c_arr(hexdata)
            for i, byte in enumerate(bytes):
                cfile.write(byte)
                if i != len(bytes) - 1:
                    cfile.write(', ')

        cfile.write('};\n\n')


    cfile.write('struct extra_lump extra_lumps[EXTRA_LUMPS_N] = {\n')

    for i, lump in enumerate(lump_files):
        cfile.write('{"' + bname(lump) + '", ' + str(stat(join(extra_lumps_folder, lump)).st_size) + ', ' + bname(lump) + '}')
        if i != len(lump_files) - 1:
            cfile.write(',\n')
        else:
            cfile.write('\n')

    cfile.write('};\n')
