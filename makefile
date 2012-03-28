CC = gcc

HEAD = rbtree.h rbtree_rc.h
SRC  = rbtree.c
DST = cfs 

# nothing: 
# 	@echo missing make tag

all: cfs 

cfs: $(HEAD) $(SRC) rbtree_rc.c cfs_sched.c
	$(CC)  $(SRC) cfs_sched.c  rbtree_rc.c -std=gnu99  -lpthread -o $@

clean:
	rm -rf cfs 
