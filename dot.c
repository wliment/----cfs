#include <gvc.h> 

int main(int argc, char **argv) 
{ 
     GVC_t  *gvc;  
     graph_t *g;  
     FILE *fp; 
     gvc = gvContext(); 

     if (argc > 1) 
          fp = fopen(argv[1], "r"); 
     else 
          fp = stdin; 
     g = agread(fp); 

     gvLayout(gvc, g, "dot"); 

     gvRender(gvc, g, "png", stdout); 

     gvFreeLayout(gvc, g); 

     agclose(g); 

     return (gvFreeContext(gvc)); 
} 