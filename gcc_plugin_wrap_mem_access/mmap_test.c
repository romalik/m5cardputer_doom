
typedef struct memblock_s
{
    //unsigned int size24:24;	// including the header and possibly tiny fragments
    unsigned int tag4:4;	// purgelevel
    unsigned int tag8:8;	// purgelevel
    void**		user;	// NULL if a free block
    struct memblock_s*	next;
    struct memblock_s*	prev;
} memblock_t;

typedef struct
{
    
    int* sector;      // Sector the SideDef is facing.

    short textureoffset; // add this to the calculated texture column
    short rowoffset;     // add this to the calculated texture top
    
    unsigned int toptexture:10;
    unsigned int bottomtexture:10;
    unsigned int midtexture:10;

} side_t;

extern side_t * hz;
extern unsigned int aaaa;
extern unsigned int bbbb;
extern unsigned int cccc;
extern unsigned int dddd;

int main() {
    bbbb = hz->toptexture;
    cccc = hz->bottomtexture;
    aaaa = hz->midtexture;

}