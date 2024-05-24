

typedef struct
{
    short		width;		// bounding box size
    short		height;
    short		leftoffset;	// pixels to the left of origin
    short		topoffset;	// pixels below the origin
    int			columnofs[8];	// only [width] used
    // the [0] is &columnofs[width]
} patch_t;

typedef struct
{
  short originx, originy;  // Block origin, which has already accounted
  const patch_t* patch;    // for the internal origin of the patch.
} texpatch_t;

typedef struct
{
  const char*  name;         // Keep name for switch changing, etc.
  //int   next, index;     // killough 1/31/98: used in hashing algorithm
  // CPhipps - moved arrays with per-texture entries to elements here
  unsigned short  widthmask;
  // CPhipps - end of additions
  short width, height;

  unsigned char overlapped;
  unsigned char patchcount;      // All the patches[patchcount] are drawn
  texpatch_t patches[1]; // back-to-front into the cached texture.
} texture_t;


    patch_t *aaa;

texture_t* texture;
int main() {

    const texpatch_t* patch = &texture->patches[j];


    int l1 = 1234;

    //int l2 = aaa->width;

    int r1 = l1 + patch->patch->width;
}
