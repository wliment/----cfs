#include "rbtree_rc.h"
#include "thread_cfs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void bst_print_dot_null(pid_t p_pid, int nullcount, FILE* stream)
{
	fprintf(stream, "    null%d [shape=point];\n", nullcount);
	fprintf(stream, "    %d -> null%d;\n",p_pid, nullcount);
}

void get_node_color(struct rb_node* node,struct sched_entity *cont,FILE* stream)
{


	printf("\n");
	printf("color is :%lu\n",rb_color(node));

	if(node->new == 1)
  {
		fprintf(stream, "    %d [shape=circle,fixedsize=true,fontcolor=red,fontsize=12,color=green,fontcolor=white,style=filled];\n", cont->p_pid);
    return;
  }
	if(rb_color(node))
		fprintf(stream, "    %d [shape=circle,fixedsize=true,fontcolor=red,fontsize=12,color=black,fontcolor=white,style=filled];\n", cont->p_pid);
	else
		fprintf(stream, "    %d [shape=circle,fixedsize=true,color=red,fontsize=12,style=filled];\n", cont->p_pid);

}
void bst_print_dot_aux(struct rb_node* node, FILE* stream)
{
	static int nullcount = 0;

	struct sched_entity *this = rb_entry(node, struct sched_entity, run_node);
	struct sched_entity *this_left = rb_entry(node->rb_left, struct sched_entity, run_node);
	struct sched_entity *this_right = rb_entry(node->rb_right, struct sched_entity, run_node);

	if (node->rb_left)
	{
		get_node_color(node,this,stream);
		fprintf(stream, "    %d -> %d;\n", this->p_pid, this_left->p_pid);
		bst_print_dot_aux(node->rb_left, stream);

	}
	else
{

		get_node_color(node,this,stream);
		bst_print_dot_null(this->p_pid, nullcount++, stream);
}

	if (node->rb_right)
	{
		get_node_color(node,this,stream);
		fprintf(stream, "    %d -> %d;\n", this->p_pid, this_right->p_pid);
		bst_print_dot_aux(node->rb_right, stream);
	}
	else
{

		get_node_color(node,this,stream);
		bst_print_dot_null(this->p_pid, nullcount++, stream);
}
}
extern void bst_print_dot(struct rb_root* root, FILE* stream)
{
	fprintf(stream, "digraph red_black_tree {\n");
  fprintf(stream, "size=\"1000,800\"  bgcolor=\"#f0ebe2\"\n");
	/*fprintf(stream, "size=\"1000,800\"  bgcolor=\"#3BFF00\"\n");*/
	fprintf(stream, "    node [fontname=\"Arial\"];\n");


	struct rb_node *tree = root->rb_node; 

	struct sched_entity *this = rb_entry(tree, struct sched_entity, run_node);
	if (!tree)
		fprintf(stream, "\n");
	else if (!tree->rb_right && !tree->rb_left)
		fprintf(stream, "    %d;\n", this->p_pid);
	else
	{

		bst_print_dot_aux(tree, stream);
	}

	fprintf(stream, "}\n");
}
	
