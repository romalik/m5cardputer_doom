#include "wadprocessor.h"
#include "doomtypes.h"
#include "mus2mid.h"

WadProcessor::WadProcessor(WadFile& wad, QObject *parent)
    : QObject(parent), wadFile(wad)
{

}

bool WadProcessor::ProcessWad()
{
    //Figure out if our IWAD is Doom or Doom2. (E1M1 or MAP01)

    RescaleImages();
    //return true;


    Lump mapLump;

    RemoveUnusedLumps();
    ConvertMusicToMIDI();


    int lumpNum = wadFile.GetLumpByName("MAP01", mapLump);

    if(lumpNum != -1)
    {
        return ProcessD2Levels();
    }
    else
    {
        int lumpNum = wadFile.GetLumpByName("E1M1", mapLump);

        //Can't find any maps.
        if(lumpNum == -1)
            return false;
    }

    return ProcessD1Levels();
}

bool WadProcessor::ProcessD2Levels()
{
    for(int m = 1; m <= 32; m++)
    {
        Lump l;

        QString mapName = QString("MAP%1").arg(m, 2, 10, QChar('0'));

        int lumpNum = wadFile.GetLumpByName(mapName, l);

        if(lumpNum != -1)
        {
            ProcessLevel(lumpNum);
        }
    }

    return true;
}

bool WadProcessor::ProcessD1Levels()
{
    for(int e = 1; e <= 4; e++)
    {
        for(int m = 1; m <= 9; m++)
        {
            Lump l;

            QString mapName = QString("E%1M%2").arg(e).arg(m);

            int lumpNum = wadFile.GetLumpByName(mapName, l);

            if(lumpNum != -1)
            {
                ProcessLevel(lumpNum);
            }
        }
    }

    return true;
}

bool WadProcessor::ProcessLevel(quint32 lumpNum)
{
    ProcessVertexes(lumpNum);
    ProcessLines(lumpNum);
    ProcessSegs(lumpNum);
    ProcessSides(lumpNum);
    ProcessPNames();

    return true;
}

bool WadProcessor::ProcessVertexes(quint32 lumpNum)
{
    quint32 vtxLumpNum = lumpNum+ML_VERTEXES;

    Lump vxl;

    if(!wadFile.GetLumpByNum(vtxLumpNum, vxl))
        return false;

    if(vxl.length == 0)
        return false;

    quint32 vtxCount = vxl.length / sizeof(mapvertex_t);

    vertex_t* newVtx = new vertex_t[vtxCount];
    const mapvertex_t* oldVtx = reinterpret_cast<const mapvertex_t*>(vxl.data.constData());

    for(quint32 i = 0; i < vtxCount; i++)
    {
        newVtx[i].x = (oldVtx[i].x << 16);
        newVtx[i].y = (oldVtx[i].y << 16);
    }

    Lump newVxl;
    newVxl.name = vxl.name;
    newVxl.length = vtxCount * sizeof(vertex_t);
    newVxl.data = QByteArray(reinterpret_cast<const char*>(newVtx), newVxl.length);

    delete[] newVtx;

    wadFile.ReplaceLump(vtxLumpNum, newVxl);

    return true;
}

bool WadProcessor::ProcessLines(quint32 lumpNum)
{
    quint32 lineLumpNum = lumpNum+ML_LINEDEFS;

    Lump lines;

    if(!wadFile.GetLumpByNum(lineLumpNum, lines))
        return false;

    if(lines.length == 0)
        return false;

    quint32 lineCount = lines.length / sizeof(maplinedef_t);

    line_t* newLines = new line_t[lineCount];

    const maplinedef_t* oldLines = reinterpret_cast<const maplinedef_t*>(lines.data.constData());

    //We need vertexes for this...

    quint32 vtxLumpNum = lumpNum+ML_VERTEXES;

    Lump vxl;

    if(!wadFile.GetLumpByNum(vtxLumpNum, vxl))
        return false;

    if(vxl.length == 0)
        return false;

    const vertex_t* vtx = reinterpret_cast<const vertex_t*>(vxl.data.constData());

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
    newLine.name = lines.name;
    newLine.length = lineCount * sizeof(line_t);
    newLine.data = QByteArray(reinterpret_cast<const char*>(newLines), newLine.length);

    delete[] newLines;

    wadFile.ReplaceLump(lineLumpNum, newLine);

    return true;
}

bool WadProcessor::ProcessSegs(quint32 lumpNum)
{
    quint32 segsLumpNum = lumpNum+ML_SEGS;

    Lump segs;

    if(!wadFile.GetLumpByNum(segsLumpNum, segs))
        return false;

    if(segs.length == 0)
        return false;

    quint32 segCount = segs.length / sizeof(mapseg_t);

    seg_t* newSegs = new seg_t[segCount];

    const mapseg_t* oldSegs = reinterpret_cast<const mapseg_t*>(segs.data.constData());

    //We need vertexes for this...

    quint32 vtxLumpNum = lumpNum+ML_VERTEXES;

    Lump vxl;

    if(!wadFile.GetLumpByNum(vtxLumpNum, vxl))
        return false;

    if(vxl.length == 0)
        return false;

    const vertex_t* vtx = reinterpret_cast<const vertex_t*>(vxl.data.constData());

    //And LineDefs. Must process lines first.

    quint32 linesLumpNum = lumpNum+ML_LINEDEFS;

    Lump lxl;

    if(!wadFile.GetLumpByNum(linesLumpNum, lxl))
        return false;

    if(lxl.length == 0)
        return false;

    const line_t* lines = reinterpret_cast<const line_t*>(lxl.data.constData());

    //And sides too...

    quint32 sidesLumpNum = lumpNum+ML_SIDEDEFS;

    Lump sxl;

    if(!wadFile.GetLumpByNum(sidesLumpNum, sxl))
        return false;

    if(sxl.length == 0)
        return false;

    const mapsidedef_t* sides = reinterpret_cast<const mapsidedef_t*>(sxl.data.constData());


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
    newSeg.name = segs.name;
    newSeg.length = segCount * sizeof(seg_t);
    newSeg.data = QByteArray(reinterpret_cast<const char*>(newSegs), newSeg.length);

    delete[] newSegs;

    wadFile.ReplaceLump(segsLumpNum, newSeg);

    return true;
}

bool WadProcessor::ProcessSides(quint32 lumpNum)
{
    quint32 sidesLumpNum = lumpNum+ML_SIDEDEFS;

    Lump sides;

    if(!wadFile.GetLumpByNum(sidesLumpNum, sides))
        return false;

    if(sides.length == 0)
        return false;

    quint32 sideCount = sides.length / sizeof(mapsidedef_t);

    sidedef_t* newSides = new sidedef_t[sideCount];

    const mapsidedef_t* oldSides = reinterpret_cast<const mapsidedef_t*>(sides.data.constData());

    for(unsigned int i = 0; i < sideCount; i++)
    {
        newSides[i].textureoffset = oldSides[i].textureoffset;
        newSides[i].rowoffset = oldSides[i].rowoffset;

        newSides[i].toptexture = GetTextureNumForName(oldSides[i].toptexture);
        newSides[i].bottomtexture = GetTextureNumForName(oldSides[i].bottomtexture);
        newSides[i].midtexture = GetTextureNumForName(oldSides[i].midtexture);

        newSides[i].sector = oldSides[i].sector;
    }

    Lump newSide;
    newSide.name = sides.name;
    newSide.length = sideCount * sizeof(sidedef_t);
    newSide.data = QByteArray(reinterpret_cast<const char*>(newSides), newSide.length);

    delete[] newSides;

    wadFile.ReplaceLump(sidesLumpNum, newSide);

    return true;
}

int WadProcessor::GetTextureNumForName(const char* tex_name)
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

    Lump tex1lump;
    wadFile.GetLumpByName("TEXTURE1", tex1lump);

    maptex1 = (const int*)tex1lump.data.constData();
    numtextures1 = *maptex1;
    directory1 = maptex1+1;

    Lump tex2lump;
    if (wadFile.GetLumpByName("TEXTURE2", tex2lump) != -1)
    {
        maptex2 = (const int*)tex2lump.data.constData();
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

        const maptexture_t* mtexture = (const maptexture_t *) ( (const quint8*)maptex + offset);

        if(!strncmp(tex_name_upper, mtexture->name, 8))
        {
            return i;
        }
    }

    return 0;
}

bool WadProcessor::ProcessPNames()
{
    Lump pnamesLump;
    qint32 lumpNum = wadFile.GetLumpByName("PNAMES", pnamesLump);

    if(lumpNum == -1)
        return false;

    const char* pnamesData = (const char*)pnamesLump.data.constData();

    quint32 count = *((quint32*)pnamesData);

    pnamesData += 4; //Fist 4 bytes are count.

    QStringList pnamesUpper;

    for(quint32 i = 0; i < count; i++)
    {
        char n[9] = {0};
        strncpy(n, &pnamesData[i*8], 8);


        QLatin1String nl(n);
        QString newName(nl);

       pnamesUpper.push_back(newName.toUpper());
    }

    char* newPnames = new char[(count * 8) + 4];
    memset(newPnames, 0, (count * 8) + 4);

    *((quint32*)newPnames) = count; //Write count of pnames.

    char* newPnames2 = &newPnames[4]; //Start of name list.

    for(quint32 i = 0; i < count; i++)
    {
        QByteArray pl = pnamesUpper[i].toLatin1();

        strncpy(&newPnames2[i*8], pl.constData(), 8);
    }

    Lump newLump;
    newLump.name = "PNAMES";
    newLump.length = (count * 8) + 4;
    newLump.data = QByteArray(reinterpret_cast<const char*>(newPnames), newLump.length);

    delete[] newPnames;

    wadFile.ReplaceLump(lumpNum, newLump);

    return true;
}

bool WadProcessor::RemoveUnusedLumps()
{

    /*
    for(quint32 i = 0; i < wadFile.LumpCount(); i++)
    {
        Lump l;

        wadFile.GetLumpByNum(i, l);

        if(     l.name.startsWith("D_") ||
                l.name.startsWith("DP") ||
                l.name.startsWith("DS") ||
                l.name.startsWith("GENMIDI"))
        {
            wadFile.RemoveLump(i);
            i--;
        }
    }
    */
    return true;
}


void rescale_flat(Lump & lump) {
    //return;


    const char * data = lump.data.constData();

    char * new_data = (char *)malloc(32*32);

    for(int r = 0; r < 64; r+=2) {
        for(int c = 0; c < 64; c+=2) {
            new_data[32*(r/2) + c/2] = data[64*r + c];
        }
    }

    lump.length = (32*32);
    lump.data = QByteArray(new_data, lump.length);
}

typedef struct image_header {
    int16_t width;
    int16_t height;
    int16_t left_off;
    int16_t top_off;
} image_header_t;

void rescale_sprite(Lump & lump) {
    //return;


    const char * data = lump.data.constData();
    const image_header_t * header = reinterpret_cast<const image_header_t*>(data);
/*
    if(header->left_off < 0) {
        //weapon sprite
        return;
    }
*/


    //printf("width %d height %d left_off %d top_off %d\n", header->width, header->height, header->left_off, header->top_off);
    std::vector<uint32_t> column_ofs;
    for(int i = 0; i<header->width; i++) {
        column_ofs.push_back(
            *reinterpret_cast<const uint32_t*>(data + sizeof(image_header_t) + i*sizeof(uint32_t))
        );
    }

/*
    printf("Offsets: ");
    for(auto o : column_ofs) {
        printf("%d ", o);
    }
    printf("\n");
*/

    char * new_data = (char*)malloc(lump.length);

    char * n_p = new_data;


    image_header_t new_header;
    new_header.height = header->height;
    new_header.width  = header->width;
    new_header.left_off = header->left_off;
    new_header.top_off  = header->top_off;

    memcpy(new_data, (char *)&new_header, sizeof(image_header_t));

    uint32_t * off_p = (uint32_t*)(new_data + sizeof(image_header_t));

    n_p += sizeof(image_header_t) + (new_header.width/2)*sizeof(uint32_t);

    for(size_t i = 0; i<column_ofs.size(); i+=2) {
        //printf("column %d\n", i);
        const unsigned char * c_col = (const unsigned char *)(data + column_ofs[i]);
        const uint8_t * c_post = c_col;

            *off_p = (uint32_t)(n_p - new_data);
            off_p++;

        while(*c_post != 0xff) {
            uint8_t post_ofs = c_post[0];
            uint8_t n_pixels = c_post[1];
            // here goes dummy byte
            const uint8_t * pixels = c_post + 3;
            //here goes dummy_byte
            c_post += 1 + 1 + 2 + n_pixels;
            //printf("post ofs %d n_pixels %d\n", post_ofs, n_pixels);


            *n_p = post_ofs; n_p++;
            *n_p = n_pixels; n_p++;
            *n_p = 0; n_p++; //dummy
            /*
            int n_new_pixels = n_pixels/2;
            if(n_pixels == 1) {
                n_new_pixels = 1;
            }
            */
            for(size_t j = 0; j < n_pixels/2; j++) {
                *n_p = pixels[j*2]; n_p++;
            }

            /*
            for(size_t j = 0; j < n_pixels/2; j++) {
                *n_p = 0; n_p++;
            }
            
            if(n_pixels & 0x01) {
                *n_p = 0; n_p++;
            }
            */
            
            *n_p = 0; n_p++; //dummy
        }
        *n_p = 0xff;
        n_p++;
    }

    lump.length = (n_p - new_data);
    lump.data = QByteArray(new_data, lump.length);
    
}

char * hud_stuff[] = {
"STGANUM",
"STYSNUM",
"STTPRCNT",
"STKEYS",
"STGNUM",
"STFST",
"STFTR",
"STFTL",
"STFOUCH",
"STFEVL",
"STFKILL",
"STFGOD0",
"STFDEAD0",
"STCFN",
"STTNUM",
"STRSNUM",
NULL
};

int is_hud_stuff(char * name) {
    char ** p = hud_stuff;
    while(*p) {
        if(!strncmp(*p, name, strlen(*p))) {
            return 1;
        }
        p++;
    }
    return 0;
}

bool WadProcessor::RescaleImages()
{

    int inside_sprite_section = 0;
    int inside_patch_section = 0;
    int inside_flat_section = 0;

    for(quint32 i = 0; i < wadFile.LumpCount(); i++)
    {
        int extra_need_rescale = 0;

        Lump l;

        wadFile.GetLumpByNum(i, l);
        
        char name[9];
        name[8] = 0;
        memcpy(name, l.name.toLatin1().toUpper().constData(), 8);

        //printf("%s\n", name);

        if(!strncmp("S_START", name, 7))
        {
            inside_sprite_section = 1;
            continue;
        }

        if(!strncmp("S_END", name, 5)) {
            inside_sprite_section = 0;
        }

        if(!strncmp("P_START", name, 7))
        {
            inside_patch_section = 1;
            continue;
        }

        if(!strncmp("P_END", name, 5)) {
            inside_patch_section = 0;
        }

        if(!strncmp("F_START", name, 7))
        {
            inside_flat_section = 1;
            continue;
        }

        if(!strncmp("F_END", name, 5)) {
            inside_flat_section = 0;
        }



        if(inside_sprite_section || inside_patch_section || is_hud_stuff(name)) {
            //printf("Sprite %s\n", name);
            Lump & l_ref = wadFile.GetLumpRef(i);
            if(l_ref.length > 0) {
                rescale_sprite(l_ref);
            }
        } else if(inside_flat_section) {
            Lump & l_ref = wadFile.GetLumpRef(i);
            if(l_ref.length > 0) {
                rescale_flat(l_ref);
            }
        } else {
            /*
            int texnum = 0;
            
            if((texnum = GetTextureNumForName(name))) {
                printf("Texture #%d: %s\n", texnum, name);
            }
            */

        }


    }
    return true;
}

bool WadProcessor::ConvertMusicToMIDI()
{


    for(quint32 i = 0; i < wadFile.LumpCount(); i++)
    {
        Lump l;

        wadFile.GetLumpByNum(i, l);

        if(l.name.startsWith("D_"))
        {

            Lump & l = wadFile.GetLumpRef(i);
            //printf("Processing %s: length %zu\n", l.name.toStdString().c_str(), l.length);

            char *mid = NULL;
            size_t midlen;
            int handle = 0;
            if (mus2mid(l.data.constData(), l.length, &mid, &midlen, 64) == 0) {
                //printf("Success! midlen = %zu\n", midlen);
                l.data = QByteArray(mid, midlen);
                l.length = midlen;
            }

        }
    }
    return true;
}
