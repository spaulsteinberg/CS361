
//COSC 361 Fall 2018
//FUSE Project Template
/*
	Author: Samuel Steinberg
	Date: March 14th, 2019	
	Collaborated with: Tanner Fry, David Clevenger
*/

#ifndef __cplusplus
#error "You must compile this using C++"
#endif
#include <fuse.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fs.h>
#include <iterator>
#include <map>
#include <fstream>
#include <utility>
#include <algorithm>
#include <dirent.h>

#define MAX_NUM_BLOCKS 100

using namespace std;

/*This struct is used to bundle a node with its number of blocks, size of one block, and its actual blocks */
struct node_t {
	NODE main;
	int num_blocks;
	int sizeof_block;
	char* blocks[MAX_NUM_BLOCKS];
};
BLOCK_HEADER *bh;
map<string, node_t *> path_to_node;
int off_position;
//Use debugf() and NOT printf() for your messages.
//Uncomment #define DEBUG in block.h if you want messages to show

//Here is a list of error codes you can return for
//the fs_xxx() functions
//
//EPERM          1      /* Operation not permitted */
//ENOENT         2      /* No such file or directory */
//ESRCH          3      /* No such process */
//EINTR          4      /* Interrupted system call */
//EIO            5      /* I/O error */
//ENXIO          6      /* No such device or address */
//ENOMEM        12      /* Out of memory */
//EACCES        13      /* Permission denied */
//EFAULT        14      /* Bad address */
//EBUSY         16      /* Device or resource busy */
//EEXIST        17      /* File exists */
//ENOTDIR       20      /* Not a directory */
//EISDIR        21      /* Is a directory */
//EINVAL        22      /* Invalid argument */
//ENFILE        23      /* File table overflow */
//EMFILE        24      /* Too many open files */
//EFBIG         27      /* File too large */
//ENOSPC        28      /* No space left on device */
//ESPIPE        29      /* Illegal seek */
//EROFS         30      /* Read-only file system */
//EMLINK        31      /* Too many links */
//EPIPE         32      /* Broken pipe */
//ENOTEMPTY     36      /* Directory not empty */
//ENAMETOOLONG  40      /* The name given is too long */

//Use debugf and NOT printf() to make your
//debug outputs. Do not modify this function.
#if defined(DEBUG)
int debugf(const char *fmt, ...)
{
	int bytes = 0;
	va_list args;
	va_start(args, fmt);
	bytes = vfprintf(stderr, fmt, args);
	va_end(args);
	return bytes;
}
#else
int debugf(const char *fmt, ...)
{
	return 0;
}
#endif

/*This function checks the first 8 bytes of the block header to make sure it is the correct header. */
static bool err_check_first_eight(BLOCK_HEADER *bh)
{
	char *n = (char *)malloc(sizeof(char)*9);	
	for (int i = 0; i < 8; i++)
	{
		n[i] = bh->magic[i];
	}
	n[8] = '\0';
	if (strcmp(n, "COSC_361") != 0) 
	{
		free(n);
		return 1;
	}
	else
	{
		free(n);
		return 0;
	}
}
//////////////////////////////////////////////////////////////////
//
// START HERE W/ fs_drive()
//
//////////////////////////////////////////////////////////////////
//Read the hard drive file specified by dname
//into memory. You may have to use globals to store
//the nodes and / or blocks.
//Return 0 if you read the hard drive successfully (good MAGIC, etc).
//If anything fails, return the proper error code (-EWHATEVER)
//Right now this returns -EIO, so you'll get an Input/Output error
//if you try to run this program without programming fs_drive.
//////////////////////////////////////////////////////////////////
int fs_drive(const char *dname)
{
	debugf("CREATE %s\n\n", dname);
	string path;
	FILE *file = fopen(dname, "rb");
	if (file == NULL) return -EIO;
	
	bh = (BLOCK_HEADER *)malloc(sizeof(BLOCK_HEADER));
	node_t *node;

	fread(bh , sizeof(BLOCK_HEADER), 1, file); //read in block header
	
	if(err_check_first_eight(bh)) return -EIO; //error check first 8 bytes

	/*Read in nodes and blocks*/
	for (unsigned int i = 0; i < bh->nodes; i++)
	{
		node = (node_t *)malloc(sizeof(node_t));
		
		fread(node, ONDISK_NODE_SIZE, 1, file);
		NODE *cur = &(node->main); //used to get data from main node
		
		node->sizeof_block = bh->block_size; /*----------Change to struct only here and in struct--------*/
		
		path = string( cur->name );
		path_to_node.insert( pair<string, node_t *>(path, node) );
		//debugf("%s    \n", cur->name);	
		if (cur->size == 0) node->num_blocks = 0;
		else //might have to do while loop up to num blocks and make offset an array
		{
			node->num_blocks = (cur->size) / bh->block_size + 1; //populate number of blocks field for node_t structure
			cur->blocks = new uint64_t[node->num_blocks];//(uint64_t *)malloc(sizeof(uint64_t)*node->num_blocks);//dynamically create space for main nodes nbr of blocks (for pointer)
			fread(cur->blocks, sizeof(uint64_t), node->num_blocks, file);
		//	debugf("offset at: %ld %ld\n", cur->blocks, *cur->blocks);
		}
	}

	int initial = ftell(file);
	/*Read in block content*/
	for (auto iter = path_to_node.begin(); iter != path_to_node.end(); ++iter)
	{
		string path = iter->first;
		node = iter->second;
		NODE *cur = &(node->main);
		//debugf("%s\n", cur->name);
		if (node->num_blocks == 0) { }
		else
		{
			for (int i = 0; i < node->num_blocks; i++)
			{
				node->blocks[i] = (char *)malloc(sizeof(char)*bh->block_size); //this blocks is to store the actual blocks ---> this allocates a block
				fseek(file, initial + ((cur->blocks[i])*bh->block_size), SEEK_SET); //get to blocks location
				fread(node->blocks[i], sizeof(char), bh->block_size, file); //read blocks into the block space in master node
	//			debugf("%s ---------> %s\n", cur->name, node->blocks[i]);
			}
		}
		
	}
	fclose(file);
//	debugf("fs_drive: %s\n", dname);
	return 0;
}

//////////////////////////////////////////////////////////////////
//Open a file <path>. This really doesn't have to do anything
//except see if the file exists. If the file does exist, return 0,
//otherwise return -ENOENT
//////////////////////////////////////////////////////////////////
/*This function will look for an existing node with the oath given. If found, open. If not, throw error. */
int fs_open(const char *path, struct fuse_file_info *fi)
{
	auto it = path_to_node.find(path); //find path based off key
	if (it == path_to_node.end()) return -ENOENT; //if not found return it doesnt exist
	//debugf("fs_open: %s\n", path);
	NODE *n = &(it->second->main);
	if ( !(n->mode & S_IFREG) ) return -EISDIR;

	return 0;
}

//////////////////////////////////////////////////////////////////
//Read a file <path>. You will be reading from the block and
//writing into <buf>, this buffer has a size of <size>. You will
//need to start the reading at the offset given by <offset> and
//write the data into *buf up to size bytes.
//////////////////////////////////////////////////////////////////
/*This function reads in the buf from the blocks*/
int fs_read(const char *path, char *buf, size_t size, off_t offset,
	    struct fuse_file_info *fi)
{
	auto it = path_to_node.find(path);
	if (it == path_to_node.end()) return -ENOENT;
	node_t *node = it->second;
	int tbr = 0, count = 0;
	debugf("number of blocks in path %s is: %d\n", it->first.c_str(), node->num_blocks);
	/*Read em into the buf */
	for (int i = 0; i < node->num_blocks; i++)
	{
		count = strlen(node->blocks[i]); //---HERE---//
		strncpy(buf + tbr, node->blocks[i], count);
		tbr += count;
	}
	debugf("bytes read is: %d\n", tbr);	
	return tbr;
}

//////////////////////////////////////////////////////////////////
//Write a file <path>. If the file doesn't exist, it is first
//created with fs_create. You need to write the data given by
//<data> and size <size> into this file block. You will also need
//to write data starting at <offset> in your file. If there is not
//enough space, return -ENOSPC. Finally, if we're a read only file
//system (fi->flags & O_RDONLY), then return -EROFS
//If all works, return the number of bytes written.
//////////////////////////////////////////////////////////////////
/*This function will write to the node blocks */
int fs_write(const char *path, const char *data, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
	debugf("fs_write: %s %d\n", path, size);
	/*If read only file system return an error */
	if (fi->flags & O_RDONLY) return -EROFS;
	auto iter = path_to_node.find(path);
	node_t *node = iter->second;
	NODE *cur = &(node->main);
	int pos = 0, count = 0;

	/*How many blocks are needed*/
	int nec_blocks = (strlen(data)/ node->sizeof_block) + 1;

	debugf("Necessary blocks %d\n", nec_blocks);

	node->num_blocks = (nec_blocks);
	cur->size += strlen(data);
	/*Create first block*/
	string h(data, bh->block_size - offset); //new
	debugf("strlen init is: %d\n", h.length());
	count += strlen(h.c_str());
	debugf("Count is %d\n", count);
	pos = count;
	debugf("pos is: %d\n", pos);	
	strncpy(node->blocks[offset/bh->block_size] + (offset % bh->block_size), h.c_str(), (bh->block_size - offset) );
	debugf("b-o: %s %d\n",node->blocks[offset/bh->block_size] + (offset % bh->block_size), strlen(node->blocks[offset/bh->block_size] + (offset % bh->block_size))); 	
	/*Create blocks after */
	for (int i = 1; i < nec_blocks; i++)
	{
		bh->blocks++;
		debugf("pos is %d\n", pos);
		string nxt(data+pos, bh->block_size);
		count += strlen(nxt.c_str());
		debugf("block: %d\n", (offset/bh->block_size + 1));
		pos = count;
		node->blocks[i] = (char *)malloc(sizeof(char)*bh->block_size);
		memset(node->blocks[i], 0, bh->block_size);
		strncpy(node->blocks[i], nxt.c_str(), bh->block_size);
	}
	debugf("write bytes: %d\n", count);
	return count;
}

//////////////////////////////////////////////////////////////////
//Create a file <path>. Create a new file and give it the mode
//given by <mode> OR'd with S_IFREG (regular file). If the name
//given by <path> is too long, return -ENAMETOOLONG. As with
//fs_write, if we're a read only file system
//(fi->flags & O_RDONLY), then return -EROFS.
//Otherwise, return 0 if all succeeds.
//////////////////////////////////////////////////////////////////
/* This function will create a file with the path passed. */
int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	/*If already exists or path name too long throw error. */
	auto iter = path_to_node.find(path);
	if (iter != path_to_node.end()) return -EEXIST;
	if (strlen(path) > NAME_SIZE) return -ENAMETOOLONG;

	node_t *node = (node_t *)malloc(sizeof(node_t));
	NODE *cur = &(node->main);
	/*set characteristics of file */
	string node_path = path;
	strcpy(cur->name,node_path.c_str());
	cur->size = 0;
	cur->mode = mode | S_IFREG;
	cur->uid = getuid();
	cur->gid = getgid();
	cur->ctime = time(NULL);
	cur->atime = time(NULL);
	cur->mtime = time(NULL);

	/*Update node information and insert to map */
	node->num_blocks = 0;
	bh->blocks++;
	bh->nodes++;
	node->sizeof_block = bh->block_size;
	node->blocks[0] = (char *)malloc(sizeof(char)*bh->block_size);
	path_to_node.insert( pair<string, node_t *>(node_path, node) );
	if ( (fi->flags & O_RDONLY) ) return -EROFS;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Get the attributes of file <path>. A static structure is passed
//to <s>, so you just have to fill the individual elements of s:
//s->st_mode = node->mode
//s->st_atime = node->atime
//s->st_uid = node->uid
//s->st_gid = node->gid
// ...
//Most of the names match 1-to-1, except the stat structure
//prefixes all fields with an st_*
//Please see stat for more information on the structure. Not
//all fields will be filled by your filesystem.
//////////////////////////////////////////////////////////////////
/*This function populates a nodes attributes field */
int fs_getattr(const char *path, struct stat *s)
{
	auto it = path_to_node.find(path); //find path based off key
	if (it == path_to_node.end()) return -ENOENT; //if not found return it doesnt exist
	
	node_t *node = it->second; //set node to the value ---> populate below
	NODE *cur = &(node->main);
	
	stat(path, s);
	
	s->st_mode = cur->mode;
	s->st_atime = cur->atime;
	s->st_ctime = cur->ctime;
	s->st_mtime = cur->mtime;
	s->st_uid = cur->uid;
	s->st_gid = cur->gid;
	s->st_nlink = 1;
	s->st_size = cur->size;

	//debugf("fs_getattr: %s\n", path);
	//debugf("mode is: %ld\n", s->st_mode);
	return 0;
}

//////////////////////////////////////////////////////////////////
//Read a directory <path>. This uses the function <filler> to
//write what directories and/or files are presented during an ls
//(list files).
//
//filler(buf, "somefile", 0, 0);
//
//You will see somefile when you do an ls
//(assuming it passes fs_getattr)
//////////////////////////////////////////////////////////////////
/*This function will read a directory, must use string manipulation to correctly enter to filler() function */
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	       off_t offset, struct fuse_file_info *fi)
{
	string old;
	int counter = 0;
	auto it = path_to_node.find(path); //find path based off key
	if (it == path_to_node.end()) return -ENOENT; //if not found return it doesnt exist
	
	filler(buf, ".", 0, 0);
	filler(buf, "..", 0, 0);

	old = path;
	int comp = count(old.begin(), old.end(), '/'); //count number of slashes
	auto h = path_to_node.lower_bound(path); //get lower bound-->should be the path in map
	for (auto iter = path_to_node.begin(); iter != path_to_node.end(); iter++)
	{
		if (h->first == iter->first) continue; //skip the actual path node
		old = iter->first;
		
		if (old.find(path) == string::npos) continue; //if path is not in file/directory go to next iteration
		if (strncmp(old.c_str(), path, strlen(path)) == 0) //compare path with first characters in nodes
		{
			counter = 0;
			for (int i = 0; i < (int)strlen(old.c_str()); i++)
			{
				if (old[i] == '/') counter++; //count number of slashed in path
			}
			if (strcmp(path, "/") == 0)//(flag == false)
			{
				if ( counter > comp) continue; //number of slashed should not exceed path slashes
				old.erase(0, strlen(path));
				filler(buf, old.c_str(), 0, 0);
			}
			else //paths with more than one slash will have children with one more slash
			{
				if ( counter > (comp + 1) ) continue;
				old.erase(0, strlen(path));
				old.erase(0, 1); //erase last slash
				filler(buf, old.c_str(), 0, 0);
			}
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////
//Open a directory <path>. This is analagous to fs_open in that
//it just checks to see if the directory exists. If it does,
//return 0, otherwise return -ENOENT
//////////////////////////////////////////////////////////////////
/*This function will open a directory, throw error if not a directory or file doesnt exist */
int fs_opendir(const char *path, struct fuse_file_info *fi)
{
	auto it = path_to_node.find(path); //find path based off key
	if (it == path_to_node.end()) return -ENOENT; //if not found return it doesnt exist
	NODE *n = &(it->second->main);
	if (!(n->mode & S_IFDIR)) return -ENOTDIR;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Change the mode (permissions) of <path> to <mode>
//////////////////////////////////////////////////////////////////
/*This function will change the permissions of a file, if path not found throw error */
int fs_chmod(const char *path, mode_t mode)
{
	auto it = path_to_node.find(path); //find path based off key
	if (it == path_to_node.end()) return -ENOENT; //if not found return it doesnt exist
	
	NODE *cur = &(it->second->main);
	cur->mode = mode;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Change the ownership of <path> to user id <uid> and group id <gid>
//////////////////////////////////////////////////////////////////
/*Change file ownership, if fails throw error. */
int fs_chown(const char *path, uid_t uid, gid_t gid)
{
	auto it = path_to_node.find(path); //find path based off key
	if (it == path_to_node.end()) return -ENOENT; //if not found return it doesnt exist
	
	NODE *cur = &(it->second->main);
	cur->uid = uid;
	cur->gid = gid;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Unlink a file <path>. This function should return -EISDIR if a
//directory is given to <path> (do not unlink directories).
//Furthermore, you will not need to check O_RDONLY as this will
//be handled by the operating system.
//Otherwise, delete the file <path> and return 0.
//////////////////////////////////////////////////////////////////
/*This function will delete a file and its contents. */
int fs_unlink(const char *path)
{
//	debugf("fs_unlink: %s\n", path);
	auto it = path_to_node.find(path);
	if (it == path_to_node.end()) return -ENOENT;
	node_t *node = it->second;
	NODE *n = &(node->main);

	if (n->mode & S_IFDIR) return -EISDIR;

	/* delete blocks from master node */
	for (int i = 0; i < node->num_blocks; i++)
	{
		free(node->blocks[i]);
		node->blocks[i] = NULL;
		bh->blocks--;
	}
	/*deallocate sub node pointer and then the sub node and then node.*/
	free(n->blocks);
	n->blocks = NULL;
	free(node);
	node = NULL;
	/*Remove key from path */
	path_to_node.erase(it);
	bh->nodes--;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Make a directory <path> with the given permissions <mode>. If
//the directory already exists, return -EEXIST. If this function
//succeeds, return 0.
//////////////////////////////////////////////////////////////////
/*This function will create a directory. If path already exists throw error. */
int fs_mkdir(const char *path, mode_t mode)
{
	auto it = path_to_node.find(path);
	if (it != path_to_node.end())
	{
		NODE *cur = &(it->second->main);
		if (cur->mode & S_IFDIR) return -EEXIST;
	}

	/*Create node and set fields */
	node_t *node = (node_t *)malloc(sizeof(node_t));
	NODE *n = &(node->main);
	string path_name = path;
	strcpy(n->name, path_name.c_str());
	n->mode = mode | S_IFDIR;
	n->size = 0;
	n->uid = getuid();
	n->gid = getgid();
	n->atime = time(NULL);
	n->ctime = time(NULL);
	n->mtime = time(NULL);
	
	node->num_blocks = 0;
	node->sizeof_block = bh->block_size;

	path_to_node.insert(pair<string, node_t *>(path_name, node));
	bh->nodes++;
	return 0;
}

//////////////////////////////////////////////////////////////////
//Remove a directory. You have to check to see if it is
//empty first. If it isn't, return -ENOTEMPTY, otherwise
//remove the directory and return 0.
//////////////////////////////////////////////////////////////////
/*This function removes an EMPTY directory */
int fs_rmdir(const char *path)
{
	string holder, temp;
	auto it = path_to_node.find(path); //find path based off key
	if (it == path_to_node.end()) return -ENOENT; //if not found return it doesnt exist
	
	node_t *node = it->second;
	NODE *n = &(node->main);
	if ( !(n->mode & S_IFDIR) ) return -ENOTDIR;
	
	/*String manipulation to make sure directory is empty before deleting */
	holder = path;
	holder.erase(0,1);
	for (auto it = path_to_node.begin(); it != path_to_node.end(); it++)
	{
		if (it->first == "/") continue;
		temp = it->first;
		temp.erase(0,1);
		if (temp == holder) continue;	
		size_t pos = temp.find(holder);
		if (pos == string::npos) continue;
		if ( (temp[pos+strlen(holder.c_str())] ==  '/') ) return -ENOTEMPTY; //check for sub files/directories
	}
	free(node);
	node = NULL;
	path_to_node.erase(it);
	bh->nodes--;

	return 0;
}

//////////////////////////////////////////////////////////////////
//Rename the file given by <path> to <new_name>
//Both <path> and <new_name> contain the full path. If
//the new_name's path doesn't exist return -ENOENT. If
//you were able to rename the node, then return 0.
//////////////////////////////////////////////////////////////////
/* This function renames a path, find if it exists first, then add the node into the map with the new key val */
int fs_rename(const char *path, const char *new_name)
{
	auto iter_old = path_to_node.find(path);
	if (iter_old == path_to_node.end()) return -ENOENT;

	auto it = path_to_node.find(new_name);
	if (it != path_to_node.end()) return -EEXIST;
	
	string ins = new_name;
	node_t *node = iter_old->second;
	NODE *n = &(node->main);
	strcpy(n->name, new_name);
	
	path_to_node.insert(pair<string, node_t *>(ins, node)); //insert new node with inheirited info
	path_to_node.erase(iter_old); //erase old one
	
	//debugf("%s\n", n->name);
	return 0;
}

//////////////////////////////////////////////////////////////////
//Reduce the size of the file <path> down to size. This will
//potentially remove blocks from the file. Make sure you properly
//handle reducing the size of a file and properly updating
//the node with the number of blocks it will take. Return 0 
//if successful
//////////////////////////////////////////////////////////////////
/*This function will truncate a file to the desired size */
int fs_truncate(const char *path, off_t size)
{
	debugf("fs_truncate: %s to size %d\n", path, size);
	auto iter = path_to_node.find(path);
	if (iter == path_to_node.end()) return -ENOENT;
	node_t *node = iter->second;
	NODE *n = &(node->main);

	if ( !(n->mode & S_IFREG) ) return -EPERM;

	int block_location = size / node->sizeof_block; //get current block;
	
	if ((int)size > (int)n->size) return 0; //if size is greater than size of node

	/* if size is less update the size */
	if ( size < node->sizeof_block )
	{
		n->size = size;
		return 0;
	}
/*Need to free blocks if size requires it */
	int cur = node->num_blocks;
	for (int i = block_location; i < cur; i++) //maybe block_location + 1 ?
	{
	//	free(node->blocks[i]);
		node->num_blocks--;
		bh->blocks--;
	}
	n->size = size;

	return 0;
}

//////////////////////////////////////////////////////////////////
//fs_destro is called when the mountpoint is unmounted
//this should save the hard drive back into <filename>
//////////////////////////////////////////////////////////////////
void fs_destroy(void *ptr)
{
	//ALSO WRITE OFFSETS BACK//
	const char *filename = (const char *)ptr;
	debugf("DESTROY: %s\n\n", filename);
	debugf("Number of nodes now at: %d\n", bh->nodes);
	debugf("Number of blocks now at: %d\n", bh->blocks);
	node_t *node;
	NODE *cur;
	uint64_t bp = 0;
	FILE *file = fopen(filename, "wb");

	fwrite(bh, sizeof(BLOCK_HEADER), 1, file);
	free(bh);

	/*Create new offsets for files */
	for (auto it = path_to_node.begin(); it != path_to_node.end(); it++)
	{
		debugf("path: %s\n", it->first.c_str());
		node = it->second;
		cur = &(node->main);
		
		if ( (cur->size) > 0 && (cur->blocks == NULL) ) cur->blocks = new uint64_t[node->num_blocks];
		
		for (int i = 0; i < node->num_blocks; i++)
		{
			cur->blocks[i] = bp;
			bp++;
		}
	}
	/*Write  nodes and offsets*/
	for (auto it = path_to_node.begin(); it != path_to_node.end(); it++)
	{
		debugf("path: %s\n", it->first.c_str());
		node = it->second;
		cur = &(node->main);
		fwrite(node, ONDISK_NODE_SIZE, 1, file);
		fwrite(cur->blocks,sizeof(uint64_t), node->num_blocks, file);
	}
	/*Write blocks to file*/
	for (auto it = path_to_node.begin(); it != path_to_node.end(); it++)
	{
		node = it->second;
		cur = &(node->main);
		if (node->num_blocks > 0)
		{
			for (int i = 0; i < node->num_blocks; i++)
			{
				debugf("%s had blocks ---->  %s", cur->name, node->blocks[i]);
				fwrite(node->blocks[i], sizeof(char), bh->block_size, file); //read blocks into the block space in master node
				free(node->blocks[i]);
			}
		}
		free(node);
	}
	fclose(file);

}

//////////////////////////////////////////////////////////////////
//int main()
//DO NOT MODIFY THIS FUNCTION
//////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	fuse_operations *fops;
	char *evars[] = { "./fs", "-f", "mnt", NULL };
	int ret;

	if ((ret = fs_drive(HARD_DRIVE)) != 0) {
		debugf("Error reading hard drive: %s\n", strerror(-ret));
		return ret;
	}
	//FUSE operations
	fops = (struct fuse_operations *) calloc(1, sizeof(struct fuse_operations));
	fops->getattr = fs_getattr;
	fops->readdir = fs_readdir;
	fops->opendir = fs_opendir;
	fops->open = fs_open;
	fops->read = fs_read;
	fops->write = fs_write;
	fops->create = fs_create;
	fops->chmod = fs_chmod;
	fops->chown = fs_chown;
	fops->unlink = fs_unlink;
	fops->mkdir = fs_mkdir;
	fops->rmdir = fs_rmdir;
	fops->rename = fs_rename;
	fops->truncate = fs_truncate;
	fops->destroy = fs_destroy;

	debugf("Press CONTROL-C to quit\n\n");

	return fuse_main(sizeof(evars) / sizeof(evars[0]) - 1, evars, fops,
			 (void *)HARD_DRIVE);
}
