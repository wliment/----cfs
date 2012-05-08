#include "thread_cfs.h"
#include <gvc.h>
int dot_to_image(FILE *dot_gv)
{
	GVC_t  *gvc;  
	graph_t *g;  
	FILE *image;
	gvc = gvContext(); 
	g = agread(dot_gv); 
	image = fopen("test.svg","w+");
	gvLayout(gvc, g, "dot"); 
	gvRender(gvc, g, "svg",image); 
	gvFreeLayout(gvc, g); 

	agclose(g); 

	fclose(image);

	return (gvFreeContext(gvc)); 

}

extern void bst_print_dot(struct rb_root* root, FILE* stream);

int if_output_tree=0;
int *tree_to_image() //将二叉树输出为文件，独立线程不对调度产生影响
{
  if(!if_output_tree)
    return 0;
  FILE *dot_gv;
  dot_gv = fopen("dot.gv","w+");
  //每隔一秒生成一次图片
  pthread_mutex_lock(&cfs_tree_mtx);
  bst_print_dot(&cfs->tasks_timeline,dot_gv);
  pthread_mutex_unlock(&cfs_tree_mtx);
  rewind(dot_gv);
  dot_to_image(dot_gv);
  ftruncate(fileno(dot_gv), 0);
  rewind(dot_gv);
  fclose(dot_gv);
  return 1;
}


