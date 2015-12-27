#define FUSE_USE_VERSION 26

#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>



typedef struct Node* Link;
typedef struct Node
{
	char* name;
	char* content;
	Link parent;
	Link* childs;
	int childCount;
} Node;

static Link tree;
static char* tempFile = "";
static char* tempContent = ""; 

void log(char* path)
{
	FILE* f = fopen("/home/OS/5/log.txt","a+");
	int i = 0;
	while(path[i] != 0) fputc((int)path[i++],f);
	fputc((int)'\n',f);
	fclose(f);
}

Link createTree()
{
	Link tree = (Link)malloc(sizeof(Node));
	tree->name = "/";
	tree->content = 0;
	tree->parent = 0;
	tree->childs = 0;
	tree->childCount = 0;
	return tree;
}

void addNode(Link parent, Link node)
{	
	node->parent = parent;	
	Link* newChilds = (Link*)malloc(sizeof(Link*)*(parent->childCount + 1));
	int i = 0;
	for(i; i < parent->childCount; i++)
	{
		newChilds[i] = parent->childs[i];
	}
	newChilds[parent->childCount] = node; 
	parent->childs = newChilds;
	parent->childCount++; 
}

Link createNode(char* name, char* content)
{
	Link node = (Link)malloc(sizeof(Node));
	node->name = name;
	node->content = content;
	node->parent = 0;
	node->childs = 0;
	node->childCount = 0;
	return node;
}

void deleteNode(Link node)
{
	Link parent = node->parent;
	Link* newChilds = (Link*)malloc(sizeof(Link)*(parent->childCount - 1));	
	int i = 0;
	int mod = 0;
	for(; i < parent->childCount; i++)
	{
		if (strcmp(parent->childs[i]->name, node->name) == 0)
		{
			mod = -1;
			continue;
		}
		newChilds[i + mod] = parent->childs[i];
	}
	parent->childCount--;
	parent->childs = newChilds;
	return 0;
}

char** split(char* path)
{
	char** array;	
	if (strlen(path) > 1)
	{	
		int count = 0;	
		int i = 0;	
		while(path[i] != 0)
		{
			if (path[i] == '/') count++;
			i++;
		}
		array = (char**)malloc(sizeof(char*)*(count + 2));
		array[count + 1] = 0;
		array[0] = "/";
		int n = 1;
		i = 1;
		while(path[i] != 0)
		{
			int c = 1;			
			int j = i;			
			while((path[j] != '/') && (path[j] != 0))
			{
				c++;j++;i++;
			}
			if (path[i] != 0) i++;
			array[n] = (char*)malloc(sizeof(char)*c);
			array[n][c - 1] = 0;
			n++;	
		}
		n = 1;
		i = 1;
		while(path[i] != 0)
		{			
			int j = i;
			int tmp = i;			
			while((path[j] != '/') && (path[j] != 0))
			{
				array[n][j - tmp] = path[j];
				j++;i++;
			}
			if (path[i] != 0) i++;
			n++;	
		}
	} else
	{
		array = (char**)malloc(sizeof(char*)*2);
		array[1] = 0;
		array[0] = "/";
	}
	return array;
}

Link skNode(Link tree, char* path)
{	
	char** splited = split(path);
	int count = 0;
	int i = 0;
	while(splited[i++] != 0) count++;
	if (count == 1) return tree;	
	Link node = tree;	
	i = 1;	
	while(splited[i] != 0)
	{		
		int j = 0;		
		for(j; j < node->childCount; j++)
		{			
			if (strcmp(node->childs[j]->name, splited[i]) == 0)
			{
				node = node->childs[j];
				break; 
			}
		}
		i++;
	}
	return node;
}

Link seekNode(Link tree, char* path)
{	
	char** splited = split(path);
	int count = 0;
	int i = 0;
	while(splited[i++] != 0) count++;
	Link node = skNode(tree,path);	
	if (strcmp(node->name,splited[count-1]) != 0) return 0;
	return node;
}

char* memcpu(char* source, char* buf)
{
	int size = strlen(source) + strlen(buf) + 1;	
	char* result = (char*)malloc(sizeof(char)*size);
	result[size - 1] = 0;
	int i = 0;
	for(; i < strlen(source); i++)
	{
		result[i] = source[i];
	}
	for(; i < size; i++)
	{
		result[i] = buf[i - strlen(source)];
	}
	return result;
}

static int my_getattr(const char *path, struct stat *stbuf)
{	
	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));
	Link node = seekNode(tree, path);
	if (node == 0) return -ENOENT;	
	if (node->content == 0)
	{
		stbuf->st_mode = S_IFDIR | 0666;
		stbuf->st_nlink = 2;
	} else
	{
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(node->content);
	}
	return 0; 
}

static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
		
	Link node = seekNode(tree, path);	
	int i = 0;
	for(; i < node->childCount; i++)
	{
		filler(buf, node->childs[i]->name, NULL, 0);
	}
	return 0;	
}

static int my_open(const char *path, struct fuse_file_info *fi)
{
	
	return 0;
}

static int my_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{		
	Link node = seekNode(tree, path);
	int len = strlen(node->content);
	if (offset < len)
	{
		if (offset + size > len) size = len - offset;
		memcpy(buf, node->content + offset, size);
	} else	size = 0;
	return size;
}

static int my_write(const char *path, const char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{	
	tempFile = path;
	tempContent = memcpu(tempContent,buf + offset);
	return strlen(buf+offset);
}

static int my_mkdir(const char* path, mode_t mode)
{	
	char** array = split(path);
	int ct = 0;	
	while(array[ct] != 0) ct++;
	Link node = createNode(array[ct - 1], 0);
	Link parent = skNode(tree, path);
	addNode(parent, node);
	return 0;		
}

static int my_mknod(const char* path, mode_t mode, dev_t dev)
{
	char** array = split(path);
	int ct = 0;	
	while(array[ct] != 0) ct++;
	Link node = createNode(array[ct - 1], "");
	Link parent = skNode(tree, path);
	addNode(parent, node);
	return 0;
}

static int my_rename(const char* old, const char* new)
{	
	if (strcmp(old,tempFile) == 0)
	{					
		Link node = seekNode(tree, new);
		node->content = tempContent;
		tempContent = "";
		return 0;
	}
	Link node = seekNode(tree, old);
	char** array = split(new);
	int i = 0;
	while(array[i] != 0) i++;
	node->name = array[i - 1];
	return 0;
}

static int my_rmdir(char* path)
{
	Link node = seekNode(tree, path);
	deleteNode(node);
	return 0;
}

static int my_unlink(char* path)
{
	Link node = seekNode(tree, path);
	deleteNode(node);
	return 0;
}

static struct fuse_operations my_oper = 
{
	.getattr	= my_getattr,//атрибуты файла
	.readdir	= my_readdir,
	.open		= my_open,
	.read		= my_read,
	.write		= my_write,	
	.mkdir 		= my_mkdir,
	.mknod		= my_mknod,
	.rename 	= my_rename,
	.rmdir		= my_rmdir,
	.unlink 	= my_unlink,
};


int main(int argc, char *argv[])
{	
	tree = createTree();
	return fuse_main(argc, argv, &my_oper, NULL);
}
