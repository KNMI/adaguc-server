
// oct1.h -- Header file for octree color quantization function
// Dean Clark
//
#ifndef OCT1_H
#define OCT1_H
typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long ulong;

#ifndef True
#define False 0
#define True 1
#endif
// RGBType is a simple 8-bit color triple
typedef struct {
  byte r, g, b, realalpha, realblue; // The color
} RGBType;
// OctnodeType is a generic octree node
typedef struct _octnode {
  int level;                       // Level for this node
  bool isleaf;                     // TRUE if this is a leaf node
  byte index;                      // Color table index
  ulong npixels;                   // Total pixels that have this color
  ulong redsum, greensum, bluesum; // Sum of the color components
  ulong realbluesum, realalphasum;
  RGBType *color;            // Color at this (leaf) node
  struct _octnode *child[8]; // Tree pointers
  struct _octnode *nextnode; // Reducible list pointer
} OctreeType;
OctreeType *CreateOctNode(int level);
void MakePaletteTable(OctreeType *tree, RGBType table[], int *index);
ulong TotalLeafNodes(void);
void ReduceTree(void);
void InsertTree(OctreeType **tree, RGBType *color, uint depth);
int QuantizeColorMapped(OctreeType *tree, RGBType *color);
int QuantizeColor(OctreeType *tree, RGBType *color);
#endif