CC = gcc

HEAD = rbtree.h rbtree_rc.h
SRC  = rbtree.c bst_to_dot.c
DST = cfs 
CFLAGS=`pkg-config libgvc --cflags` `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0` -std=gnu99  -lpthread  -O2 
LDFLAGS=`pkg-config libgvc --libs` 
# nothing: 
# 	@echo missing make tag

all: cfs 

cfs: $(HEAD) $(SRC) rbtree_rc.c cfs_sched.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) cfs_sched.c  rbtree_rc.c  -o $@

clean:
	rm -rf cfs 
