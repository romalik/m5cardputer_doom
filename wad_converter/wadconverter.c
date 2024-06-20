

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "doomtypes.h"


struct writer {
    int (*write)(void*, size_t);
    int (*pos)();
    int (*seek)(int offset);
};


struct free_list_entry {
    void * ptr;
    struct free_list_entry * next;
};

struct free_list_entry * free_list = NULL;

void add_free_list(void * ptr) {
    return;


    struct free_list_entry ** e = &free_list;
    while(*e != NULL) {
        e = &((*e)->next);
    }
    *e = (struct free_list_entry *)malloc(sizeof(struct free_list_entry));
    (*e)->next = NULL;
    (*e)->ptr = ptr;
}

void do_free_list() {
    return;


    struct free_list_entry ** e = &free_list;
    while(*e != NULL) {
        struct free_list_entry * c = *e;
        e = &((*e)->next);

        free(c->ptr);
        free(c);
    }   
}


void print_free_list() {
    printf("Free list: \n");

    struct free_list_entry ** e = &free_list;
    while(*e != NULL) {
        struct free_list_entry * c = *e;
        e = &((*e)->next);

        printf("  @%p = %p next %p\n", c, c->ptr, c->next);
    }   
    

    printf("-------\n");
}

typedef struct Lump_s
{
    char * name;
    uint32_t length;
    char * data;
} Lump;


Lump * lumps = NULL;
size_t n_lumps = 0;

void print_lumps() {
    for(size_t i = 0; i < n_lumps; i++) {
        printf("%ld: %.8s %d\n", i, lumps[i].name, lumps[i].length);
    }
}

int32_t get_lump_by_name(char * name, Lump ** lump)
{
    for(int i = n_lumps - 1; i >= 0; i--)
    {
        if(!strncmp(lumps[i].name, name, 8))
        {
            (*lump) = &lumps[i];
            return i;
        }
    }

    return -1;
}

int get_lump_by_num(uint32_t lumpnum, Lump ** lump)
{
    if(lumpnum >= n_lumps)
        return 0;

    (*lump) = &lumps[lumpnum];

    return 1;
}


int replace_lump(uint32_t lumpnum, Lump * newLump)
{
    if(lumpnum >= n_lumps)
        return 0;

    lumps[lumpnum].data = newLump->data;
    lumps[lumpnum].length = newLump->length;
    lumps[lumpnum].name = newLump->name;

    return 1;
}


int get_texture_num_for_name(const char* tex_name)
{
    const int  *maptex1, *maptex2;
    int  numtextures1, numtextures2 = 0;
    const int *directory1, *directory2;


    //Convert name to uppercase for comparison.
    char tex_name_upper[9];

    strncpy(tex_name_upper, tex_name, 8);
    tex_name_upper[8] = 0; //Ensure null terminated.

    for (int i = 0; i < 8; i++)
    {
        tex_name_upper[i] = toupper(tex_name_upper[i]);
    }

    Lump * tex1lump;
    get_lump_by_name("TEXTURE1", &tex1lump);

    maptex1 = (int*)(tex1lump->data);
    numtextures1 = *maptex1;
    directory1 = maptex1+1;

    Lump * tex2lump;
    if (get_lump_by_name("TEXTURE2", &tex2lump) != -1)
    {
        maptex2 = (int*)(tex2lump->data);
        directory2 = maptex2+1;
        numtextures2 = *maptex2;
    }
    else
    {
        maptex2 = NULL;
        directory2 = NULL;
    }

    const int *directory = directory1;
    const int *maptex = maptex1;

    int numtextures = (numtextures1 + numtextures2);

    for (int i=0 ; i<numtextures; i++, directory++)
    {
        if (i == numtextures1)
        {
            // Start looking in second texture file.
            maptex = maptex2;
            directory = directory2;
        }

        int offset = *directory;

        const maptexture_t* mtexture = (const maptexture_t *) ( (const uint8_t*)maptex + offset);

        if(!strncmp(tex_name_upper, mtexture->name, 8))
        {
            return i;
        }
    }

    return 0;
}

int process_vertices(uint32_t lumpNum)
{
    uint32_t vtxLumpNum = lumpNum+ML_VERTEXES;

    Lump * vxl;

    if(!get_lump_by_num(vtxLumpNum, &vxl))
        return 0;

    if(vxl->length == 0)
        return 0;

    uint32_t vtxCount = vxl->length / sizeof(mapvertex_t);

    vertex_t* newVtx = (vertex_t *)malloc(vtxCount*sizeof(vertex_t));

    mapvertex_t* oldVtx = (mapvertex_t*)(vxl->data);

    for(uint32_t i = 0; i < vtxCount; i++)
    {
        newVtx[i].x = (oldVtx[i].x << 16);
        newVtx[i].y = (oldVtx[i].y << 16);
    }

    Lump newVxl;
    newVxl.name = vxl->name;
    newVxl.length = vtxCount * sizeof(vertex_t);
    newVxl.data = (char*)(newVtx);

    add_free_list(newVtx);

    replace_lump(vtxLumpNum, &newVxl);

    return 1;
}

int process_lines(uint32_t lumpNum)
{
    uint32_t lineLumpNum = lumpNum+ML_LINEDEFS;

    Lump * lines;

    if(!get_lump_by_num(lineLumpNum, &lines))
        return 0;

    if(lines->length == 0)
        return 0;

    uint32_t lineCount = lines->length / sizeof(maplinedef_t);

    line_t* newLines = (line_t *)malloc(sizeof(line_t) * lineCount);

    const maplinedef_t* oldLines = (maplinedef_t*)(lines->data);

    //We need vertexes for this...

    uint32_t vtxLumpNum = lumpNum+ML_VERTEXES;

    Lump * vxl;

    if(!get_lump_by_num(vtxLumpNum, &vxl))
        return 0;

    if(vxl->length == 0)
        return 0;

    const vertex_t* vtx = (vertex_t*)(vxl->data);

    for(unsigned int i = 0; i < lineCount; i++)
    {
        newLines[i].v1.x = vtx[oldLines[i].v1].x;
        newLines[i].v1.y = vtx[oldLines[i].v1].y;

        newLines[i].v2.x = vtx[oldLines[i].v2].x;
        newLines[i].v2.y = vtx[oldLines[i].v2].y;

        newLines[i].special = oldLines[i].special;
        newLines[i].flags = oldLines[i].flags;
        newLines[i].tag = oldLines[i].tag;

        newLines[i].dx = newLines[i].v2.x - newLines[i].v1.x;
        newLines[i].dy = newLines[i].v2.y - newLines[i].v1.y;

        newLines[i].slopetype =
                !newLines[i].dx ? ST_VERTICAL : !newLines[i].dy ? ST_HORIZONTAL :
                FixedDiv(newLines[i].dy, newLines[i].dx) > 0 ? ST_POSITIVE : ST_NEGATIVE;

        newLines[i].sidenum[0] = oldLines[i].sidenum[0];
        newLines[i].sidenum[1] = oldLines[i].sidenum[1];

        newLines[i].bbox[BOXLEFT] = (newLines[i].v1.x < newLines[i].v2.x ? newLines[i].v1.x : newLines[i].v2.x);
        newLines[i].bbox[BOXRIGHT] = (newLines[i].v1.x < newLines[i].v2.x ? newLines[i].v2.x : newLines[i].v1.x);

        newLines[i].bbox[BOXTOP] = (newLines[i].v1.y < newLines[i].v2.y ? newLines[i].v2.y : newLines[i].v1.y);
        newLines[i].bbox[BOXBOTTOM] = (newLines[i].v1.y < newLines[i].v2.y ? newLines[i].v1.y : newLines[i].v2.y);

        newLines[i].lineno = i;

    }

    Lump newLine;
    newLine.name = lines->name;
    newLine.length = lineCount * sizeof(line_t);
    newLine.data = (char*)(newLines);

    add_free_list(newLines);

    replace_lump(lineLumpNum, &newLine);

    return 1;
}

int process_segs(uint32_t lumpNum)
{
    uint32_t segsLumpNum = lumpNum+ML_SEGS;

    Lump * segs;

    if(!get_lump_by_num(segsLumpNum, &segs))
        return 0;

    if(segs->length == 0)
        return 0;

    uint32_t segCount = segs->length / sizeof(mapseg_t);

    seg_t* newSegs = (seg_t *)malloc(sizeof(seg_t) * segCount);

    const mapseg_t* oldSegs = (mapseg_t*)(segs->data);

    //We need vertexes for this...

    uint32_t vtxLumpNum = lumpNum+ML_VERTEXES;

    Lump * vxl;

    if(!get_lump_by_num(vtxLumpNum, &vxl))
        return 0;

    if(vxl->length == 0)
        return 0;

    vertex_t* vtx = (vertex_t*)(vxl->data);

    //And LineDefs. Must process lines first.

    uint32_t linesLumpNum = lumpNum+ML_LINEDEFS;

    Lump * lxl;

    if(!get_lump_by_num(linesLumpNum, &lxl))
        return 0;

    if(lxl->length == 0)
        return 0;

    line_t* lines = (line_t*)(lxl->data);

    //And sides too...

    uint32_t sidesLumpNum = lumpNum+ML_SIDEDEFS;

    Lump * sxl;

    if(!get_lump_by_num(sidesLumpNum, &sxl))
        return 0;

    if(sxl->length == 0)
        return 0;

    mapsidedef_t* sides = (mapsidedef_t*)(sxl->data);


    //****************************

    for(unsigned int i = 0; i < segCount; i++)
    {
        newSegs[i].v1.x = vtx[oldSegs[i].v1].x;
        newSegs[i].v1.y = vtx[oldSegs[i].v1].y;

        newSegs[i].v2.x = vtx[oldSegs[i].v2].x;
        newSegs[i].v2.y = vtx[oldSegs[i].v2].y;

        newSegs[i].angle = oldSegs[i].angle << 16;
        newSegs[i].offset = oldSegs[i].offset << 16;

        newSegs[i].linenum = oldSegs[i].linedef;

        const line_t* ldef = &lines[newSegs[i].linenum];

        int side = oldSegs[i].side;

        newSegs[i].sidenum = ldef->sidenum[side];

        if(newSegs[i].sidenum != NO_INDEX)
        {
            newSegs[i].frontsectornum = sides[newSegs[i].sidenum].sector;
        }
        else
        {
            newSegs[i].frontsectornum = NO_INDEX;
        }

        newSegs[i].backsectornum = NO_INDEX;

        if(ldef->flags & ML_TWOSIDED)
        {
            if(ldef->sidenum[side^1] != NO_INDEX)
            {
                newSegs[i].backsectornum = sides[ldef->sidenum[side^1]].sector;
            }
        }
    }

    Lump newSeg;
    newSeg.name = segs->name;
    newSeg.length = segCount * sizeof(seg_t);
    newSeg.data = (char*)(newSegs);

    add_free_list(newSegs);

    replace_lump(segsLumpNum, &newSeg);

    return 1;
}

int process_sides(uint32_t lumpNum)
{
    uint32_t sidesLumpNum = lumpNum+ML_SIDEDEFS;

    Lump * sides;

    if(!get_lump_by_num(sidesLumpNum, &sides))
        return 0;

    if(sides->length == 0)
        return 0;

    uint32_t sideCount = sides->length / sizeof(mapsidedef_t);

    sidedef_t* newSides = (sidedef_t *)malloc(sizeof(sidedef_t) * sideCount);

    mapsidedef_t* oldSides = (mapsidedef_t*)(sides->data);

    for(unsigned int i = 0; i < sideCount; i++)
    {
        newSides[i].textureoffset = oldSides[i].textureoffset;
        newSides[i].rowoffset = oldSides[i].rowoffset;

        newSides[i].toptexture = get_texture_num_for_name(oldSides[i].toptexture);
        newSides[i].bottomtexture = get_texture_num_for_name(oldSides[i].bottomtexture);
        newSides[i].midtexture = get_texture_num_for_name(oldSides[i].midtexture);

        newSides[i].sector = oldSides[i].sector;
    }

    Lump newSide;
    newSide.name = sides->name;
    newSide.length = sideCount * sizeof(sidedef_t);
    newSide.data = (char*)(newSides);

    add_free_list(newSides);

    replace_lump(sidesLumpNum, &newSide);

    return 1;
}



int process_pnames() {
    printf("process_pnames() STUB\n");    
    return 1;
}


int process_level(uint32_t lumpNum)
{
    printf("Process level @ %d\n", lumpNum);

    if(!process_vertices(lumpNum))  return 1;
    if(!process_lines(lumpNum))     return 1;
    if(!process_segs(lumpNum))      return 1;
    if(!process_sides(lumpNum))     return 1;
    if(!process_pnames())           return 1;

    return 0;
}



int process_d2_levels()
{
    for(int m = 1; m <= 32; m++)
    {
        Lump * l;

        char map_name[9];
        sprintf(map_name, "MAP%02d", m);

        int lump_num = get_lump_by_name(map_name, &l);

        if(lump_num != -1) {
            process_level(lump_num);
        } else {
            printf("Lump %s not found\n", map_name);
        }
    }

    return 0;
}

int process_d1_levels()
{
    for(int e = 1; e <= 4; e++)
    {
        for(int m = 1; m <= 9; m++)
        {
            Lump * l;

            char map_name[9];
            sprintf(map_name, "E%dM%d", e,m);

            int lump_num = get_lump_by_name(map_name, &l);

            if(lump_num != -1) {
                process_level(lump_num);
            } else {
                printf("Lump %s not found\n", map_name);
            }
        }
    }

    return 0;
}

void remove_unused_lumps() {
    printf("remove_unused_lumps() STUB\n");
}

void convert_mus_to_midi() {
    printf("convert_mus_to_midi() STUB\n");
}


int process_wad() {
    Lump * map_lump;
    int is_doom2 = 0;
    int is_doom1 = 0;

    
    remove_unused_lumps();
    convert_mus_to_midi();


    int lumpNum = get_lump_by_name("MAP01", &map_lump);

    if(lumpNum != -1)
    {
        is_doom2 = 1;
    }
    else
    {
        int lumpNum = get_lump_by_name("E1M1", &map_lump);

        if(lumpNum != -1) {
            is_doom1 = 1;
        }
    }

    if(is_doom1) {
        return process_d1_levels();
    } else if(is_doom2) {
        return process_d2_levels();
    } else {
        return 1;
    }
}

#define ROUND_UP4(x) ((x+3) & -4)

int save_wad(struct writer * wad_writer)
{
    wadinfo_t header;

    header.numlumps = n_lumps;

    header.identification[0] = 'I';
    header.identification[1] = 'W';
    header.identification[2] = 'A';
    header.identification[3] = 'D';

    header.infotableofs = sizeof(wadinfo_t);

    wad_writer->write((char*)(&header), sizeof(header));

    uint32_t fileOffset = sizeof(wadinfo_t) + (sizeof(filelump_t)*n_lumps);

    fileOffset = ROUND_UP4(fileOffset);

    //Write the file info blocks.
    for(int i = 0; i < n_lumps; i++)
    {
        Lump * l = &lumps[i];

        filelump_t fl;

        memset(fl.name, 0, 8);
        strncpy(fl.name, l->name, 8);

        fl.size = l->length;

        if(l->length > 0)
            fl.filepos = fileOffset;
        else
            fl.filepos = 0;

        wad_writer->write((char*)(&fl), sizeof(fl));

        char name[9];
        name[8] = 0;
        memcpy(name, fl.name, 8);

        fileOffset += l->length;
        fileOffset = ROUND_UP4(fileOffset);
    }

    //Write the lump data out.
    for(int i = 0; i < n_lumps; i++)
    {
        Lump * l = &lumps[i];

        if(l->length == 0)
            continue;

        uint32_t pos = wad_writer->pos();

        pos = ROUND_UP4(pos);

        wad_writer->seek(pos);

        wad_writer->write(l->data, l->length);
    }

    return 1;
}


#include "extra_lumps.h"

int wadconverter_main(char * wad, size_t wad_size, struct writer * wad_writer)
{
    const wadinfo_t* header = (wadinfo_t*)wad;

    if(memcmp((char *)header, "IWAD", 4)) {
        printf("Bad wad header\n");
        return 1;
    }

    filelump_t* fileinfo = (filelump_t*)(&wad[header->infotableofs]);
    n_lumps = header->numlumps + EXTRA_LUMPS_N;

    lumps = (Lump*)malloc(n_lumps * sizeof(Lump));

    //load main wad lumps
    for(int i = 0; i < header->numlumps; i++)
    {
        Lump * l = &lumps[i];
        l->name = fileinfo[i].name;
        l->length = fileinfo[i].size;
        l->data = &wad[fileinfo[i].filepos];
    }

    //inject additional wad lumps
    for(int i = 0; i < EXTRA_LUMPS_N; i++) {
        Lump * l = &lumps[i + header->numlumps];
        l->name = extra_lumps[i].name;
        l->length = extra_lumps[i].length;
        l->data = extra_lumps[i].data;
    }

    process_wad();

    save_wad(wad_writer);

    free(lumps);
}


FILE * wadwriter_file = NULL;
int wad_write(void * buf, size_t size) {
    return fwrite(buf, size, 1, wadwriter_file);
}

int wad_pos() {
    return ftell(wadwriter_file);
}

int wad_seek(int offset) {
    return fseek(wadwriter_file, offset, SEEK_SET);
}

int main(int argc, char ** argv) {

    char * file_in = NULL;
    char * file_out = NULL;

    if(argc < 3) {
        return 1;
    }

    file_in = argv[1];
    file_out = argv[2];

    FILE * wad_fd = fopen(file_in, "r");

    fseek(wad_fd, 0, SEEK_END);
    size_t wad_size = ftell(wad_fd);
    rewind(wad_fd);

    char * wad = (char *)malloc(wad_size);
    fread(wad, wad_size, 1, wad_fd);

    fclose(wad_fd);

    FILE * outwad_fd = fopen(file_out, "wb");
 
    wadwriter_file = outwad_fd;

    struct writer wad_writer;

    wad_writer.write = &wad_write;
    wad_writer.pos   = &wad_pos;
    wad_writer.seek  = &wad_seek;

    wadconverter_main(wad, wad_size, &wad_writer);
    fclose(outwad_fd);
    free(wad);
    do_free_list();


    return 0;

}