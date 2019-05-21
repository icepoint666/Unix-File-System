#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>

using namespace std;

#define SYSTEM_SIZE 16 * 1024 * 1024
#define FREE_SIZE 16 * 1000 * 1024
#define BLOCK_SIZE 1024
#define MAX_NAME_SIZE 28
#define DIR_SIZE 16
#define MAX_DIR_DEPTH 10
#define INODE_NUM 640
#define SUPER_BLOCK_NUM 4
#define FREE_BLOCK_NUM FREE_SIZE / BLOCK_SIZE
#define BLOCKS_PER_GROUP 64

#define DIR_TYPE 0
#define FILE_TYPE 1

#define SYSTEM_FILE_NAME "UFS.sys"

struct SuperBlock{ // Use 20 blocks, Block 0~19
    unsigned int ssize;
    unsigned int itsize;
    unsigned short next_free_inode;
    unsigned short num_free_inode;
    unsigned short next_free_block;
    unsigned short num_free_block;
    bool inode_bitmap[INODE_NUM];
    bool block_bitmap[FREE_BLOCK_NUM];

};

struct DirItem{
    char item_name[MAX_NAME_SIZE];
    unsigned short item_inode_id;
};

struct INode{ // Use 364 blocks, Block 20-383
    int inode_type;
    int inode_num_link;
    struct DirItem dir_items[DIR_SIZE];
    int dir_size;
    int inode_size;
    time_t create_time;
    int inode_addr[10];
    int inode_indirect_addr;
};

struct FileSystem{
    struct SuperBlock root;
    struct INode inode[INODE_NUM];
    char free_space[FREE_SIZE];
};

char content[(10 + BLOCK_SIZE / 3) * BLOCK_SIZE];

struct FileSystem *root = new FileSystem;

char PATH[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";

enum Status{
    SUCCESS,
    ERROR_PATH_FAILED,
    ERROR_FILE_EXIST,
    ERROR_FILE_NOT_EXIST,
    ERROR_FILE_FAILED,
    ERROR_OVERLOAD
};

int get_inode(char *path){
    if (path[0] != '/'){
        return -1;
    }
    else{
        struct INode cinode = root->inode[0];
        int id = 0;
        char cpath[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
        strcpy(cpath, path);
        char *fpath = strtok(cpath, "/");
        //cout << fpath;
        int match = 0;
        while (fpath != NULL){
            match = 0;
            //cout << fpath;
            for (int i = 0; i < DIR_SIZE; i++){
                if (!strncmp(fpath, cinode.dir_items[i].item_name, max(strlen(fpath), strlen(cinode.dir_items[i].item_name)))){
                    if (root->inode[cinode.dir_items[i].item_inode_id].inode_type == DIR_TYPE){
                        id = cinode.dir_items[i].item_inode_id;
                        cinode = root->inode[id];
                        match = 1;
                        break;
                    }
                    else{
                        id = cinode.dir_items[i].item_inode_id;
                        return id;
                    }
                }
                if(i == DIR_SIZE - 1){
                    return -1;
                }
            }
            fpath = strtok(NULL, "/");
        }
        return id;
    }
}

int get_parent_inode(char *path){
    char p_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
    char pname[MAX_NAME_SIZE] = "";
    strcpy(p_path, path);
    char *fpath = strtok(p_path, "/");
    strcpy(p_path, "");
    bool f_flag = false;
    while (fpath != NULL){
        if(f_flag){
            strcat(p_path, "/");
            strcat(p_path, pname);
        }else{
            f_flag = true;
        }
        strcpy(pname, fpath);
        fpath = strtok(NULL, "/");
    }

    return get_inode(p_path);
}

Status createDir(char *path, char* pname){
    // nested-directory supported
    char p_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
    if (pname[0] == '/'){
        strcpy(p_path, pname);
    }else {
        strcpy(p_path, path);
        strcat(p_path, "/");
        strcat(p_path, pname);
    }
    int tmp_id = get_inode(p_path);
    if (tmp_id != -1){
        cout << "Directory has exists!" << endl;
        return ERROR_PATH_FAILED;
    }

    char o_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
    strcpy(o_path, p_path);

    char *fpath = strtok(p_path, "/");
    strcpy(p_path, "");
    strcpy(pname, "");
    bool f_flag = false;
    while (fpath != NULL){
        if(f_flag){
            strcat(p_path, "/");
            strcat(p_path, pname);
        }else{
            f_flag = true;
        }
        strcpy(pname, fpath);
        fpath = strtok(NULL, "/");
    }

    int inode_id = get_inode(p_path);
    if (inode_id == -1){
        cout << "Wrong Path!" << endl;
        return ERROR_PATH_FAILED;
    }
    int inode_loc;
    bool flag = false;
    for (int i = 0; i < INODE_NUM; i++)
        if (root->inode[i].inode_num_link != 1){
            inode_loc = i;
            root->inode[i].inode_type = DIR_TYPE;
            root->inode[i].inode_size = 0;
            root->inode[i].create_time = time(NULL);
            root->inode[i].dir_size = 0;
            root->inode[i].inode_addr[0] = NULL;
            root->inode[i].inode_num_link = 1;
            for (int j = 0; j < DIR_SIZE; j++){
                if (strlen(root->inode[inode_id].dir_items[j].item_name) == 0) {
                    root->inode[inode_id].dir_items[j].item_inode_id = inode_loc;
                    root->inode[inode_id].dir_size++;
                    strcpy(root->inode[inode_id].dir_items[j].item_name, pname);
                    flag = true;
                    break;
                }
            }
            if(!flag){
                root->inode[i].inode_num_link = 0;
            }
            break;
        }
    if (flag){
        cout << "Create " << o_path << " Successfully!" << endl;
        return SUCCESS;
    }
    else{
        return ERROR_OVERLOAD;
    }
}

Status changeDir(char *topath){

    if (!strcmp(topath, "..")){
        char path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
        int len;
        for (int i = strlen(PATH); i >= 0; i--){
            if (PATH[i] == '/'){
                len = i;
                break;
            }
        }
        strncpy(path, PATH, len);
        strcpy(PATH, path);
    }
    else{
        char p_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
        if (topath[0] == '/'){
            strcpy(p_path, topath);
        }else {
            strcpy(p_path, PATH);
            strcat(p_path, "/");
            strcat(p_path, topath);
        }
        if (get_inode(p_path) == -1 || get_inode(p_path) == 0){
            cout << "Wrong directory!" << endl;
            return ERROR_FILE_NOT_EXIST;
        }
        else{
            strcpy(PATH, p_path);
        }
        //cout << path;
    }
    return SUCCESS;
}

bool include_itself(char *dest_path, char *cur_path){
    if (strstr(cur_path, dest_path)!=NULL){
        return true;
    }
    return false;
}

Status createFile(char *path, char *filename, int size){
    if(size < 0){
        cout << "Invalid Size!" << endl;
        return ERROR_OVERLOAD;
    }
    char p_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
    if (filename[0] == '/'){
        strcpy(p_path, filename);
    }else {
        strcpy(p_path, path);
        strcat(p_path, "/");
        strcat(p_path, filename);
    }
    int tmp_id = get_inode(p_path);
    if (tmp_id != -1){
        cout << "File has exists!" << endl;
        return ERROR_PATH_FAILED;
    }
    char o_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
    strcpy(o_path, p_path);

    char *fpath = strtok(p_path, "/");
    strcpy(p_path, "");
    strcpy(filename, "");
    bool f_flag = false;
    while (fpath != NULL){
        if(f_flag){
            strcat(p_path, "/");
            strcat(p_path, filename);
        }else{
            f_flag = true;
        }
        strcpy(filename, fpath);
        fpath = strtok(NULL, "/");
    }

    int inode_id = get_inode(p_path);
    if (inode_id == -1){
        cout << "Wrong Path!" << endl;
        return ERROR_PATH_FAILED;
    }
    int inode_loc;
    srand((unsigned)time(NULL));
    bool flag = false;
    for (int i = 0; i < INODE_NUM; i++)
        if (root->inode[i].inode_num_link != 1) {
            inode_loc = i;
            root->inode[i].inode_type = FILE_TYPE;
            root->inode[i].inode_size = size;
            root->inode[i].create_time = time(NULL);
            root->inode[i].dir_size = 0;
            root->inode[i].inode_addr[0] = NULL;
            root->inode[i].inode_num_link = 1;
            int num_block = 0;
            if (size <= BLOCK_SIZE / 3) {
                if (size <= 10) {
                    num_block = size;
                }else{
                    num_block = size + 1;
                }
                int free_block = 0;
                for(int j = 0; j < FREE_BLOCK_NUM; j++){
                    if(!root->root.block_bitmap[j])
                        free_block++;
                    if(free_block==num_block)
                        break;
                }
                if(free_block!=num_block){
                    cout << "No Enough Free Space!" << endl;
                    root->inode[i].inode_num_link = 0;
                    root->inode[i].inode_size = 0;
                    return ERROR_OVERLOAD;
                }
            }
            else{
                cout << "Size is too large!" << endl;
                root->inode[i].inode_num_link = 0;
                root->inode[i].inode_size = 0;
                return ERROR_OVERLOAD;
            }
            memset(content, '\0', sizeof(content));
            for (int j = 0; j < DIR_SIZE; j++){
                if (strlen(root->inode[inode_id].dir_items[j].item_name) == 0) {
                    int use_block = 0;
                    for(int k = 0; k < FREE_BLOCK_NUM; k++){
                        if(!root->root.block_bitmap[k]){
                            if(use_block < 10){
                                root->inode[i].inode_addr[use_block] = k;
                                root->root.block_bitmap[k] = true;

                                //fill value
                                char fill_value[BLOCK_SIZE];
                                for (int l = 0; l < BLOCK_SIZE; l++){
                                    fill_value[l] = rand()%26+65;
                                }
                                memcpy(root->free_space+(k*BLOCK_SIZE), fill_value, BLOCK_SIZE);
                                memcpy(content + (use_block * BLOCK_SIZE), fill_value, BLOCK_SIZE);

                            }else if(use_block == 10){
                                root->inode[i].inode_indirect_addr = k;
                                root->root.block_bitmap[k] = true;
                            }else{
                                root->root.block_bitmap[k] = true;
                                char tmp[4];
                                int pos = k * BLOCK_SIZE;
                                memcpy(tmp, &pos, 4);
                                int indirect_pos = root->inode[i].inode_indirect_addr * BLOCK_SIZE + (use_block - 11) * 3;
                                root->free_space[indirect_pos] = tmp[0];
                                root->free_space[indirect_pos+1] = tmp[1];
                                root->free_space[indirect_pos+2] = tmp[2];

                                //fill value
                                char fill_value[BLOCK_SIZE];
                                for (int l = 0; l < BLOCK_SIZE; l++){
                                    fill_value[l] = rand()%26+65;
                                }
                                memcpy(root->free_space+(k*BLOCK_SIZE), fill_value, BLOCK_SIZE);
                                memcpy(content + ((use_block - 1) * BLOCK_SIZE), fill_value, BLOCK_SIZE);
                            }
                            use_block++;
                        }
                        if(use_block == num_block){
                            break;
                        }
                    }
                    cout << content << endl;
                    root->inode[inode_id].dir_items[j].item_inode_id = inode_loc;
                    root->inode[inode_id].dir_size++;
                    strcpy(root->inode[inode_id].dir_items[j].item_name, filename);
                    flag = true;
                    break;
                }
            }
            if(!flag){
                root->inode[i].inode_num_link = 0;
                root->inode[i].inode_size = 0;
            }
            break;
        }
    if (flag){
        cout << "Create " << o_path << " Successfully!" << endl;
        return SUCCESS;
    }
    else{
        return ERROR_OVERLOAD;
    }
}

Status cp(char *path, char *pname_1, char *pname_2){
    char p_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
    if (pname_1[0] == '/'){
        strcpy(p_path, pname_1);
    }else {
        strcpy(p_path, path);
        strcat(p_path, "/");
        strcat(p_path, pname_1);
    }
    int inode_id = get_inode(p_path);
    if (inode_id == -1){
        cout << "File doesn't exist!" << endl;
        return ERROR_PATH_FAILED;
    }
    if(root->inode[inode_id].inode_type == DIR_TYPE){
        cout << "The copy object is not a file!" << endl;
        return ERROR_PATH_FAILED;
    }

    char dst_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
    if (pname_2[0] == '/'){
        strcpy(dst_path, pname_2);
    }else {
        strcpy(dst_path, path);
        strcat(dst_path, "/");
        strcat(dst_path, pname_2);
    }
    int tmp_id = get_inode(dst_path);
    if (tmp_id != -1){
        cout << "File has exists!" << endl;
        return ERROR_PATH_FAILED;
    }
    char o_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
    strcpy(o_path, dst_path);

    char *fpath = strtok(dst_path, "/");
    strcpy(dst_path, "");
    strcpy(pname_2, "");
    bool f_flag = false;
    while (fpath != NULL){
        if(f_flag){
            strcat(dst_path, "/");
            strcat(dst_path, pname_2);
        }else{
            f_flag = true;
        }
        strcpy(pname_2, fpath);
        fpath = strtok(NULL, "/");
    }
    int path_id = get_inode(dst_path);
    if (path_id == -1){
        cout << "Wrong Path!" << endl;
        return ERROR_PATH_FAILED;
    }

    int inode_loc;
    srand((unsigned)time(NULL));
    bool flag = false;
    for (int i = 0; i < INODE_NUM; i++)
        if (root->inode[i].inode_num_link != 1) {
            inode_loc = i;
            root->inode[i].inode_type = FILE_TYPE;
            root->inode[i].inode_size = root->inode[inode_id].inode_size;
            root->inode[i].create_time = time(NULL);
            root->inode[i].dir_size = 0;
            root->inode[i].inode_addr[0] = NULL;
            root->inode[i].inode_num_link = 1;
            int size = root->inode[inode_id].inode_size;
            int num_block = 0;
            if (size <= BLOCK_SIZE / 3) {
                if (size <= 10) {
                    num_block = size;
                }else{
                    num_block = size + 1;
                }
                int free_block = 0;
                for(int j = 0; j < FREE_BLOCK_NUM; j++){
                    if(!root->root.block_bitmap[j])
                        free_block++;
                    if(free_block==num_block)
                        break;
                }
                if(free_block!=num_block){
                    cout << "No Enough Free Space!" << endl;
                    root->inode[i].inode_num_link = 0;
                    root->inode[i].inode_size = 0;
                    return ERROR_OVERLOAD;
                }
            }
            else{
                cout << "Size is too large!" << endl;
                root->inode[i].inode_num_link = 0;
                root->inode[i].inode_size = 0;
                return ERROR_OVERLOAD;
            }
            for (int j = 0; j < DIR_SIZE; j++){
                if (strlen(root->inode[path_id].dir_items[j].item_name) == 0) {
                    int use_block = 0;
                    for(int k = 0; k < FREE_BLOCK_NUM; k++){
                        if(!root->root.block_bitmap[k]){
                            if(use_block < 10){
                                root->inode[i].inode_addr[use_block] = k;
                                root->root.block_bitmap[k] = true;
                                memcpy(root->free_space+(k*BLOCK_SIZE), root->free_space + (root->inode[inode_id].inode_addr[use_block] * BLOCK_SIZE), BLOCK_SIZE);
                            }else if(use_block == 10){
                                root->inode[i].inode_indirect_addr = k;
                                root->root.block_bitmap[k] = true;
                            }else{
                                root->root.block_bitmap[k] = true;
                                char tmp[4];
                                int pos = k * BLOCK_SIZE;
                                memcpy(tmp, &pos, 4);
                                int indirect_pos = root->inode[i].inode_indirect_addr * BLOCK_SIZE + (use_block - 11) * 3;
                                root->free_space[indirect_pos] = tmp[0];
                                root->free_space[indirect_pos+1] = tmp[1];
                                root->free_space[indirect_pos+2] = tmp[2];

                                int source_pos = root->inode[inode_id].inode_indirect_addr * BLOCK_SIZE + (use_block - 11) * 3;
                                int addr;
                                char tp[4];
                                tp[0] = root->free_space[indirect_pos];
                                tp[1] = root->free_space[indirect_pos+1];
                                tp[2] = root->free_space[indirect_pos+2];
                                tp[3] = '\0';
                                memcpy(&addr, tp, 4);
                                memcpy(root->free_space+(k*BLOCK_SIZE), root->free_space + addr, BLOCK_SIZE);
                            }
                            use_block++;
                        }
                        if(use_block == num_block){
                            break;
                        }
                    }
                    cout << endl;
                    root->inode[path_id].dir_items[j].item_inode_id = inode_loc;
                    root->inode[path_id].dir_size++;
                    strcpy(root->inode[path_id].dir_items[j].item_name, pname_2);
                    flag = true;
                    break;
                }
            }
            if(!flag){
                root->inode[i].inode_num_link = 0;
                root->inode[i].inode_size = 0;
            }
            break;
        }
    if (flag){
        cout << "Copy to " << o_path << " Successfully!" << endl;
        return SUCCESS;
    }
    else{
        return ERROR_OVERLOAD;
    }

}

Status cat(char *path, char *pname){
    char p_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
    if (pname[0] == '/'){
        strcpy(p_path, pname);
    }else {
        strcpy(p_path, path);
        strcat(p_path, "/");
        strcat(p_path, pname);
    }
    int inode_id = get_inode(p_path);
    if (inode_id == -1){
        cout << "File doesn't exist!" << endl;
        return ERROR_PATH_FAILED;
    }
    int size = root->inode[inode_id].inode_size;
    memset(content, '\0', sizeof(content));
    for (int i = 0; i < size; i++){
        if(i < 10){
            memcpy(content + (i * BLOCK_SIZE), root->free_space + (root->inode[inode_id].inode_addr[i] * BLOCK_SIZE), BLOCK_SIZE);
        }else{
            int indirect_pos = root->inode[inode_id].inode_indirect_addr * BLOCK_SIZE + (i - 10) * 3;
            int addr;
            char tmp[4];
            tmp[0] = root->free_space[indirect_pos];
            tmp[1] = root->free_space[indirect_pos+1];
            tmp[2] = root->free_space[indirect_pos+2];
            tmp[3] = '\0';
            memcpy(&addr, tmp, 4);
            //cout << addr << endl;
            memcpy(content + (i * BLOCK_SIZE), root->free_space + addr, BLOCK_SIZE);
        }
    }
    cout << content << endl;
}

Status deleteFile(char *path, char *pname){
    // nested-directory supported
    char p_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
    if (pname[0] == '/'){
        strcpy(p_path, pname);
    }else {
        strcpy(p_path, path);
        strcat(p_path, "/");
        strcat(p_path, pname);
    }

    char o_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
    strcpy(o_path, p_path);

    int inode_id = get_inode(p_path);
    if (inode_id == -1){
        cout << "File doesn't exist!" << endl;
        return ERROR_PATH_FAILED;
    }

    if(root->inode[inode_id].inode_type == DIR_TYPE){
        cout << "It is not a file!" << endl;
        return ERROR_PATH_FAILED;
    }else{
        root->inode[inode_id].inode_num_link = 0;
        int num_block;
        int size = root->inode[inode_id].inode_size;
        root->inode[inode_id].inode_size = 0;
        if (size <= 10){
            for (int i = 0; i < size; i++){
                root->root.block_bitmap[root->inode[inode_id].inode_addr[i]] = false;
                root->inode[inode_id].inode_addr[i] = NULL;
            }
        }else{
            for (int i = 0; i < 10; i++){
                root->root.block_bitmap[root->inode[inode_id].inode_addr[i]] = false;
                root->inode[inode_id].inode_addr[i] = NULL;
            }
            int indirect_addr = root->inode[inode_id].inode_indirect_addr;
            root->root.block_bitmap[indirect_addr] = false;
            for (int i = 0; i < size - 10; i++){
                int addr;
                int indirect_pos = indirect_addr * BLOCK_SIZE + i * 3;
                char tmp[4];
                tmp[0] = root->free_space[indirect_pos];
                tmp[1] = root->free_space[indirect_pos+1];
                tmp[2] = root->free_space[indirect_pos+2];
                tmp[3] = '\0';
                memcpy(&addr, tmp, 4);
                root->root.block_bitmap[addr / BLOCK_SIZE] = false;
            }
        }
        int parent_id = get_parent_inode(p_path);
        for(int i = 0; i < DIR_SIZE; i++){
            if(root->inode[parent_id].dir_items[i].item_inode_id == inode_id){
                strcpy(root->inode[parent_id].dir_items[i].item_name, "");
                root->inode[parent_id].dir_items[i].item_inode_id = 0;
                root->inode[parent_id].dir_size--;
                break;
            }
            if(i == DIR_SIZE - 1){
                cout << "Can't find parent directory!" << endl;
                return ERROR_PATH_FAILED;
            }
        }
        cout << "Remove " << o_path << " Successfully!" << endl;
        return SUCCESS;
    }

}

Status deleteDir(char *path, char *pname){
    // nested-directory supported
    char p_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
    if (pname[0] == '/'){
        strcpy(p_path, pname);
    }else {
        strcpy(p_path, path);
        strcat(p_path, "/");
        strcat(p_path, pname);
    }

    char o_path[MAX_NAME_SIZE * MAX_DIR_DEPTH] = "";
    strcpy(o_path, p_path);

    int inode_id = get_inode(p_path);
    if (inode_id == -1){
        cout << "Directory doesn't exist!" << endl;
        return ERROR_PATH_FAILED;
    }
    // keep from removing itself
    if (include_itself(p_path, path)){
        cout << "Can't remove current directory!" << endl;
        return ERROR_PATH_FAILED;
    }

    if(root->inode[inode_id].dir_size > 0){
        cout << "Directory is not empty!" << endl;
        return ERROR_PATH_FAILED;
    }else if(root->inode[inode_id].inode_type == FILE_TYPE){
        cout << "It is not a directory!" << endl;
        return ERROR_PATH_FAILED;
    }
    else{
        root->inode[inode_id].inode_num_link = 0;
        int parent_id = get_parent_inode(p_path);
        for(int i = 0; i < DIR_SIZE; i++){
            if(root->inode[parent_id].dir_items[i].item_inode_id == inode_id){
                strcpy(root->inode[parent_id].dir_items[i].item_name, "");
                root->inode[parent_id].dir_items[i].item_inode_id = 0;
                root->inode[parent_id].dir_size--;
                break;
            }
            if(i == DIR_SIZE - 1){
                cout << "Can't find parent directory!" << endl;
                return ERROR_PATH_FAILED;
            }
        }
        cout << "Remove " << o_path << " Successfully!" << endl;
        return SUCCESS;
    }
}

Status dir(char *path){
    int inode_id = get_inode(path);
    tm* local;
    char buf[128]= {0};
    cout << setw(10) << "NAME" << setw(5) << "TYPE" << setw(6) << "SIZE" << setw(21) << "TIME" << endl;
    for (int i = 0; i < DIR_SIZE; i++){
        if (strlen(root->inode[inode_id].dir_items[i].item_name) != 0){
            cout << setw(10) << root->inode[inode_id].dir_items[i].item_name;
            if (root->inode[root->inode[inode_id].dir_items[i].item_inode_id].inode_type == DIR_TYPE){
                cout << setw(5) << "DIR" << setw(6) << "-";
            }
            else{
                cout << setw(5) << "FILE" << setw(6) << root->inode[root->inode[inode_id].dir_items[i].item_inode_id].inode_size;
            }
            time_t create_time = root->inode[root->inode[inode_id].dir_items[i].item_inode_id].create_time;
            local = localtime(&create_time);
            strftime(buf, 64, "%Y-%m-%d %H:%M:%S", local);
            cout << setw(21) << buf;
            cout << endl;
        }
    }
    return SUCCESS;
}

void sum(){
    cout << "SUM BLOCKS: " << SYSTEM_SIZE / BLOCK_SIZE << endl;
    cout << "SUPER BLOCKS: " << 20 << endl;
    cout << "INODE BLOCKS: " << 364 << endl;
    int use_blocks = 0;
    for (int i = 0; i < FREE_BLOCK_NUM; i++){
        if(root->root.block_bitmap[i])
            use_blocks++;
    }
    cout << "USE BLOCKS / FREE BLOCKS: " << use_blocks << " / " << SYSTEM_SIZE / BLOCK_SIZE - 384 << endl;
}

void initialize_file_system(){
    memset(&root->root, '\0', 20 * 1024);
    memset(root->inode, '\0', 364 * 1024);
    root->inode[0].dir_items[0].item_inode_id = 1;
    root->inode[0].create_time = time(NULL);
    root->inode[0].dir_size = 1;
    strcpy(root->inode[0].dir_items[0].item_name, "root");
    root->inode[0].inode_type = DIR_TYPE;
    root->inode[0].inode_num_link = 1;
    root->inode[1].inode_type = DIR_TYPE;
    root->inode[1].inode_num_link = 1;
    root->inode[1].create_time = time(NULL);
    root->inode[1].dir_size = 0;
    root->root.num_free_block = FREE_BLOCK_NUM;
    strcpy(PATH, "/root");
}


int main(){
    string command;
    char arg1[MAX_NAME_SIZE * 5];
    int arg2;
    char arg3[MAX_NAME_SIZE * 5];
    cout << "  Welcome to Unix File System!  " << endl;
    cout << endl;
    cout << "================================" << endl;
    cout << "     Developers Information     " << endl;
    cout << "================================" << endl;
    cout << "    Student A  -- 2016305XXXXX  " << endl;
    cout << "    Student B  -- 2016305XXXXX  " << endl;
    cout << "    Student C  -- 2016305XXXXX  " << endl;
    cout << "================================" << endl;
    cout << endl;
    FILE *fr;
    FILE *fw;
    if ((fr = fopen(SYSTEM_FILE_NAME, "rb")) == NULL) {
        initialize_file_system();
    }else{
        // load file system
        fr = fopen(SYSTEM_FILE_NAME, "rb");
        fread(&(root->root), sizeof(root->root), 1, fr);
        fseek(fr, 20 * 1024, 0);
        fread(&(root->inode), sizeof(root->inode), 1, fr);
        fseek(fr, 384 * 1024, 0);
        fread(&(root->free_space), sizeof(root->free_space), 1, fr);
        fclose(fr);
        strcpy(PATH, "/root");
    }
    while(1){
        cin >> command;
        if (command == "createDir"){
            cin >> arg1;
            createDir(PATH, arg1);
        }
        else if (command == "dir"){
            dir(PATH);
        }
        else if (command == "changeDir"){
            cin >> arg1;
            changeDir(arg1);
        }
        else if (command == "deleteDir"){
            cin >> arg1;
            deleteDir(PATH, arg1);
        }
        else if (command == "exit"){
            // save file system
            fw = fopen(SYSTEM_FILE_NAME, "wb");
            fwrite(&(root->root), sizeof(root->root), 1, fw);
            fseek(fw, 20 * 1024, 0);
            fwrite(&(root->inode), sizeof(root->inode), 1, fw);
            fseek(fw, 384 * 1024, 0);
            fwrite(&(root->free_space), sizeof(root->free_space), 1, fw);
            fclose(fw);
            return 0;
        }
        else if (command == "createFile"){
            cin >> arg1 >> arg2;
            createFile(PATH, arg1, arg2);
        }
        else if (command == "cat"){
            cin >> arg1;
            cat(PATH, arg1);
        }
        else if (command == "deleteFile"){
            cin >> arg1;
            deleteFile(PATH, arg1);
        }
        else if (command == "cp"){
            cin >> arg1 >> arg3;
            cp(PATH, arg1, arg3);
        }
        else if (command == "sum"){
            sum();
        }
        else{
            cout << "Wrong Command!" << endl;
        }
    }
}
