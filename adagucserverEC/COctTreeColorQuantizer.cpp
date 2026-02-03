#include "COctTreeColorQuantizer.h"
// oct1.c -- Color octree routines.
// Dean Clark
//
#include <cstdio>
#include <cstdlib>

#define COLORBITS 8
#define TREEDEPTH 6
byte MASK[COLORBITS] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
#define BIT(b, n) (((b) & MASK[n]) >> n)
#define LEVEL(c, d) ((BIT((c)->r, (d))) << 2 | BIT((c)->g, (d)) << 1 | BIT((c)->b, (d)))
OctreeType *reducelist[TREEDEPTH + 1]; // List of reducible nodes
static byte glbLeafLevel = TREEDEPTH;
static uint glbTotalLeaves = 0;
static void *getMem(size_t size);
static void MakeReducible(int level, OctreeType *node);
static OctreeType *GetReducible(void);
// InsertTree -- Insert a color into the octree
//
void InsertTree(OctreeType **tree, RGBType *color, uint depth) {
  //   int level;
  if (*tree == (OctreeType *)NULL) {
    *tree = CreateOctNode(depth);
  }
  if ((*tree)->isleaf) {
    (*tree)->npixels++;
    (*tree)->redsum += color->r;
    (*tree)->greensum += color->g;
    (*tree)->bluesum += color->b;
    (*tree)->realbluesum += color->realblue;
    (*tree)->realalphasum += color->realalpha;
  } else {
    InsertTree(&((*tree)->child[LEVEL(color, TREEDEPTH - depth)]), color, depth + 1);
  }
}
// ReduceTree -- Combines all the children of a node into the parent,
// makes the parent into a leaf
//
void ReduceTree() {
  OctreeType *node;
  ulong sumred = 0, sumgreen = 0, sumblue = 0, sumrealalpha = 0, sumrealblue = 0;
  byte i, nchild = 0;
  node = GetReducible();
  for (i = 0; i < COLORBITS; i++) {
    if (node->child[i]) {
      nchild++;
      sumred += node->child[i]->redsum;
      sumgreen += node->child[i]->greensum;
      sumblue += node->child[i]->bluesum;
      sumrealalpha += node->child[i]->realalphasum;
      sumrealblue += node->child[i]->realbluesum;

      node->npixels += node->child[i]->npixels;
      free(node->child[i]);
    }
  }
  node->isleaf = True;
  node->redsum = sumred;
  node->greensum = sumgreen;
  node->bluesum = sumblue;
  node->realalphasum = sumrealalpha;
  node->realbluesum = sumrealblue;
  glbTotalLeaves -= (nchild - 1);
}
// CreateOctNode -- Allocates and initializes a new octree node.  The level
// of the node is determined by the caller.
// Arguments:  level   int     Tree level where the node will be inserted.
// Returns:    Pointer to newly allocated node.  Does not return on failure.
//
OctreeType *CreateOctNode(int level) {
  static OctreeType *newnode;
  int i;
  newnode = (OctreeType *)getMem(sizeof(OctreeType));
  newnode->level = level;
  newnode->isleaf = level == glbLeafLevel;
  if (newnode->isleaf) {
    glbTotalLeaves++;
  } else {
    MakeReducible(level, newnode);
  }
  newnode->npixels = 0;
  newnode->index = 0;
  newnode->npixels = 0;
  newnode->redsum = newnode->greensum = newnode->bluesum = newnode->realbluesum = newnode->realalphasum = 0L;
  for (i = 0; i < COLORBITS; i++) {
    newnode->child[i] = NULL;
  }
  return newnode;
}
// MakeReducible -- Adds a node to the reducible list for the specified level
//
static void MakeReducible(int level, OctreeType *node) {
  node->nextnode = reducelist[level + 1];
  reducelist[level + 1] = node;
}
// GetReducible -- Returns next available reducible node at tree's leaf level
//
static OctreeType *GetReducible(void) {
  OctreeType *node;

  while (reducelist[glbLeafLevel] == NULL) {
    glbLeafLevel--;
  }
  node = reducelist[glbLeafLevel];
  reducelist[glbLeafLevel] = reducelist[glbLeafLevel]->nextnode;
  return node;
}
// MakePaletteTable -- Given a color octree, traverse tree and:
//  - Add the averaged RGB leaf color to the color palette table;
//  - Store the palette table index in the tree;
// When this recursive function finally returns, 'index' will contain
// the total number of colors in the palette table.
//
void MakePaletteTable(OctreeType *tree, RGBType table[], int *index) {
  int i;
  if (tree->isleaf) {
    table[*index].r = (byte)(tree->redsum / tree->npixels);
    table[*index].g = (byte)(tree->greensum / tree->npixels);
    table[*index].b = (byte)(tree->bluesum / tree->npixels);
    table[*index].realblue = (byte)(tree->realbluesum / tree->npixels);
    table[*index].realalpha = (byte)(tree->realalphasum / tree->npixels);

    tree->index = *index;
    //  if(*index>100)return;
    (*index)++;
  } else {
    for (i = 0; i < COLORBITS; i++) {
      if (tree->child[i]) {
        MakePaletteTable(tree->child[i], table, index);
      }
    }
  }
}
// QuantizeColor -- Returns palette table index of an RGB color by traversing
// the octree to the leaf level
//

int lastColor = -1;
int lastIndex = 0;

int QuantizeColorMapped(OctreeType *tree, RGBType *color) {
  // return QuantizeColor(tree,color);;;
  int key = color->r + color->g * 256 + color->b * 65536;
  if (lastColor == key) {
    return lastIndex;
  }
  lastColor = key;
  lastIndex = QuantizeColor(tree, color);
  ;
  return lastIndex;
}

int QuantizeColor(OctreeType *tree, RGBType *color) {

  if (tree->isleaf) {
    return tree->index;
  }
  return QuantizeColor(tree->child[LEVEL(color, 6 - tree->level)], color);
}
// TotalLeafNodes -- Returns the total leaves in the tree (glbTotalLeaves)
//
ulong TotalLeafNodes() { return glbTotalLeaves; }
// getMem -- Memory allocation routine
//
static void *getMem(size_t size) {
  void *mem;
  mem = (void *)malloc(size);
  if (mem == NULL) {
    printf("Error allocating %ld bytes in getMem\n", (ulong)size);
    exit(-1);
  }
  return mem;
}