#include <stdio.h>

#include "gpc.h"
//#include <gpc.c>

int main(void)
{
  gpc_polygon subject, clip, result;
  FILE *sfp, *cfp, *ofp;

  printf("2\n");

  sfp= fopen("TEST_tree.gpf", "r");
  cfp= fopen("TEST_kachel.gpf", "r");

  //sfp= fopen("/home/heiner/flo/gpct101/polygons/britain.gpf", "r");
  //cfp= fopen("/home/heiner/flo/gpct101/polygons/arrows.gpf", "r");

  printf("3\n");

  gpc_read_polygon(sfp, 0, &subject);

  printf("1\n");

  gpc_read_polygon(cfp, 0, &clip);

  printf("4\n");

  gpc_polygon_clip(GPC_INT, &subject, &clip, &result);

  printf("5\n");

  ofp= fopen("ERGEBNIS", "w");
  gpc_write_polygon(ofp, 0, &result);

  printf("6\n");

  gpc_free_polygon(&subject);
  gpc_free_polygon(&clip);
  gpc_free_polygon(&result);

  printf("7\n");

  fclose(sfp);
  fclose(cfp);
  fclose(ofp);
  return 0;
}
