#include <stdio.h>
#include "gpc.h"
#include <gpc.c>
int main(void)
{
  gpc_polygon subject, clip, result;
  FILE *sfp, *cfp, *ofp;

  sfp= fopen("TEST_tree.gfp", "r");
  cfp= fopen("TEST_kachel.gfp", "r");
  gpc_read_polygon(sfp, 0, &subject);
  gpc_read_polygon(cfp, 0, &clip);

  gpc_polygon_clip(GPC_INT, &subject, &clip, &result);

  ofp= fopen("ERGEBNIS", "w");
  gpc_write_polygon(ofp, 0, &result);

  gpc_free_polygon(&subject);
  gpc_free_polygon(&clip);
  gpc_free_polygon(&result);

  fclose(sfp);
  fclose(cfp);
  fclose(ofp);
  return 0;
}