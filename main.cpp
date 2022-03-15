#include <bits/stdc++.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#define clearScreen() cout<<"\033c";
#define RESET   "\033[0m"
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
using namespace std;

//-------------------------------------SIZE DEFINITION------------------------------------------------------------------
#define DISK_SIZE 500*1000*1000
#define BLOCK_SIZE 5000
#define NUM_BLOCKS 100000
#define NO_OF_INODES 6000
#define NO_OF_FD 32


#define nullfile  "/*NULL*/"
//-------------------------------------GLOBAL CLASSES-------------------------------------------------------------------

string current_disk="";
bool mount_status= false;

class inode
{
public:
    int filesize;
    int pointer[10];
    char filename[30];
    char mode;

    inode()
    {
        strcpy(filename,nullfile);
        filesize=0;

        for(int i=0;i<10;i++)
        {
            pointer[i]=-1;
        }

    }


    void clearblock(int i)
    {
        if(pointer[i]==-1)
        {
            return ;
        }


        int inum = pointer[i];

        int out=open(current_disk.c_str(),O_WRONLY|O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IXOTH);

        pwrite(out,0,BLOCK_SIZE,inum*BLOCK_SIZE);

        close(out);

    }


    void clearinode()
    {
        for(int i=0;i<10;i++)
        {

            if(pointer[i]!=-1)
            {
                clearblock(i);
                pointer[i]=-1;
            }

        }
    }


    int lastblock()
    {
        int i;
        for(i=0;i<10;i++)
        {
            if(pointer[i]==-1)
            {
                break;
            }
        }

        if(i==0)
        {
            return i;
        }
        else
        {
            return i-1;
        }
    }

};

class Super_Block
{
public:
    int blocks_for_sb ;
    int pointer_to_inodes;
    int blocks_for_inodes;
    int free_blocks;
    int pointer_to_blocks;
    int last_inode;
    int last_block;

    char free_inodes_map[NO_OF_INODES];
    char free_blocks_map[NUM_BLOCKS];

    Super_Block()
    {
        blocks_for_sb = ceil((float)(sizeof(Super_Block)/BLOCK_SIZE));
        pointer_to_inodes = blocks_for_sb;
        blocks_for_inodes = ceil((float)NO_OF_INODES*sizeof(inode)/BLOCK_SIZE);
        free_blocks = (DISK_SIZE/BLOCK_SIZE) - blocks_for_sb - blocks_for_inodes;
        pointer_to_blocks = blocks_for_inodes+blocks_for_sb;
        last_block=pointer_to_blocks;
        last_inode=0;


        for(int i=0;i<NO_OF_INODES;i++)
        {
            free_inodes_map[i]='0';
        }
        for(int i=0;i<NUM_BLOCKS;i++)
        {
            if(i<pointer_to_blocks)
            {
                free_blocks_map[i]='1';
            }
            else
            {
                free_blocks_map[i]='0';
            }
        }
    }
};






inode current_inode_arr[NO_OF_INODES];
Super_Block current_sb;
map<string,int> filename_to_inode;
map<string,string> fd_to_filename;
map<string,char> fd_mode;
set<string>opened_fd;
set<string>opened_files;



//-----------------------------------------------------------------------------------------------

void create_disk(string destination)
{
    int out=open(destination.c_str(),O_WRONLY|O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IXOTH);
    if(out<0)
    {
        cout<<BOLDRED<<"ERROR: NO FILE CREATED"<<RESET<<endl;
        return;
    }

    pwrite(out," ",1,DISK_SIZE);

    Super_Block temp_sb;
    inode temp_arr[NO_OF_INODES];
    pwrite(out,(char*)&temp_sb,sizeof (temp_sb),0);

    pwrite(out,(char*)temp_arr, sizeof(inode)*NO_OF_INODES, temp_sb.pointer_to_inodes*BLOCK_SIZE);



    close(out);

    cout<<BOLDGREEN<<"DISK CREATION SUCCESSFUL"<<RESET<<endl;
}
//------------------------MOUNT DISK------------------------------------------------------------------------------------


void mount_disk(string dest)
{
    int in = open(dest.c_str(),O_RDONLY);

    if(in<0)
    {
        cout<<BOLDRED<<"ERROR: NO SUCH DISK FOUND"<<RESET<<endl;
        exit(0);
    }

    int len;

    memset((char*)&current_sb,0,sizeof(Super_Block));
    memset((char*)current_inode_arr,0,(sizeof(inode))*NO_OF_INODES);
    len=pread(in, (char*)&current_sb, sizeof(Super_Block),0);

    len= pread(in, (char*)current_inode_arr,(sizeof(inode))*NO_OF_INODES, current_sb.pointer_to_inodes*BLOCK_SIZE);

    for(int i=0;i<NO_OF_INODES;i++)
    {
        if(strcmp(current_inode_arr[i].filename,nullfile) != 0)
        {
            filename_to_inode[string(current_inode_arr[i].filename)] = i;
        }
    }


    close(in);

    mount_status=true;
    current_disk=dest;


}

void dismount_disk()
{
    cout<<endl;

    if(mount_status == false)
    {
        cout<<BOLDRED<<"NO DISK CURRENTLY MOUNTED"<<RESET<<endl;
        return;
    }
    int out=open(current_disk.c_str(),O_WRONLY|O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IXOTH);
    if(out<0)
    {
        cout<<BOLDRED<<"DISMOUNT UNSUCCESSFUL"<<RESET<<endl;
        return;
    }

    pwrite(out,(char*)&current_sb,sizeof (current_sb),0);

    pwrite(out,(char*)current_inode_arr, sizeof(inode)*NO_OF_INODES, current_sb.pointer_to_inodes*BLOCK_SIZE);

    close(out);

    current_disk="";
    mount_status=false;

    filename_to_inode.clear();
    fd_to_filename.clear();
    fd_mode.clear();
    opened_fd.clear();
    opened_files.clear();


}

//--------------------------------------------------------------------------------------------
void open_file(string filename)
{
    if(filename_to_inode.find(filename)==filename_to_inode.end())
    {
        cout<<filename<<BOLDRED<<" NOT PRESENT IN THE DISK"<<RESET<<endl;
        return;

    }

    if(opened_files.find(filename)!=opened_files.end())
    {
        cout<<filename<<BOLDRED<<" IS ALREADY OPENED"<<RESET<<endl;
        return ;
    }

    int ch;
    cout<<BOLDBLUE<<"0. READ MODE\n1. WRITE MODE\n2. APPEND MODE\n"<<BOLDYELLOW<<"ENTER FILE MODE : "<<RESET;
    cin>>ch;
    char mode;
    string md;
    switch (ch) {
        case 0: {
            mode = 'r';
            md = "READ";
            break;
        }
        case 1:{
            mode = 'w';
            md = "WRITE";
            break;
        }
        case 2:{
            mode = 'a';
            md = "APPEND";
            break;
        }
        default:{
            cout<<BOLDRED<<"INVALID MODE SELECTED"<<RESET<<endl;
            return ;
        };
    }


    string file_fd="fd_"+filename.substr(0,2)+"_";
    file_fd.push_back(mode);

    opened_fd.insert(file_fd);
    opened_files.insert(filename);

    fd_to_filename[file_fd]=filename;
    fd_mode[file_fd]=mode;

    cout<<BOLDGREEN<<"FILE "<<BOLDYELLOW<<filename<<BOLDGREEN<<" OPENED IN "<<BOLDYELLOW<<md<<BOLDGREEN<<" MODE\n"<<BOLDGREEN<<"FILE DESCRIPTOR : "<<BOLDYELLOW<<file_fd<<RESET<<endl;


}



void write_helper(string dest, string str, int n)
{
    char buff[str.length()];


    strcpy(buff, str.c_str());

    int out=open(dest.c_str(),O_WRONLY|O_CREAT,S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IXOTH);

    pwrite(out,buff,str.length(),n);

    close(out);
}

string read_helper(string dest, int n)
{
    char buff[BLOCK_SIZE];

    int len;

    int in = open(dest.c_str(),O_RDONLY);


    len= pread(in,buff,BLOCK_SIZE,n);

    cout<<buff;

    close(in);

    return string(buff);
}


//--------------------------------------------FILE OPERATIONS-----------------------------------------------------------

void create_file(string filename)
{

    if(filename_to_inode.find(filename)!=filename_to_inode.end())
    {
        cout<<BOLDRED<<"FILE WITH THIS NAME ALREADY EXISTS"<<RESET<<endl;
        return;
    }

    int inum =current_sb.last_inode++;
    current_sb.free_inodes_map[inum]='1';

    inode cur;
    strcpy(cur.filename,filename.c_str());
    current_inode_arr[inum]=cur;

    filename_to_inode[filename]=inum;

    cout<<BOLDGREEN<<"FILE CREATION SUCCESSFUL!"<<RESET<<endl;

}


void write_file(string fd)
{
    if(fd_to_filename.find(fd) == fd_to_filename.end())
    {
        cout<<BOLDRED<<"INVALID FILE DESCRIPTOR SELECTED"<<RESET<<endl;
        return;
    }

    if(fd_mode[fd] != 'w')
    {
        cout<<BOLDRED<<"FILE IS NOT IN WRITE MODE"<<RESET<<endl;
        return;
    }

    string filename = fd_to_filename[fd];
    int inum = filename_to_inode[filename];

    cout<<BOLDYELLOW<<"ENTER FILE CONTENT YOU WANT TO WRITE :"<<RESET<<endl;

    string content="",temp="";
    while(true){
        content += "\n";
        getline(cin>>ws, temp);
        if(temp == ":q")
            break;
        content += temp;
    }


    int fsize = sizeof (content);

    int blocks_needed = ceil((float)(fsize/BLOCK_SIZE));

    if(blocks_needed==0 && content.length()!=0)
    {
        blocks_needed=1;
    }

    if(blocks_needed>10)
    {
        cout<<BOLDRED"FILE SIZE EXCEEDED"<<RESET<<endl;
        return;
    }

    if(blocks_needed>current_sb.free_blocks)
    {
        cout<<BOLDRED<<"NOT ENOUGH SPACE IN DISK"<<RESET<<endl;
        return;
    }


    current_inode_arr[inum].filesize = fsize;
    current_inode_arr[inum].clearinode();
    for(int i = 0; i<blocks_needed;i++) //change
    {

        current_sb.free_blocks_map[current_sb.last_block]='1';
        current_sb.free_blocks--;

        current_inode_arr[inum].pointer[i]= current_sb.last_block++;


        string tempcontent= content.substr(i*BLOCK_SIZE,BLOCK_SIZE);

        write_helper(current_disk,tempcontent,current_inode_arr[inum].pointer[i]*BLOCK_SIZE);


    }



    cout<<BOLDGREEN<<"WRITE SUCCESSFUL!!"<<RESET<<endl;

}


void append_file(string fd)
{
    if(fd_to_filename.find(fd) == fd_to_filename.end())
    {
        cout<<BOLDRED<<"INVALID FILE DESCRIPTOR SELECTED"<<RESET<<endl;
        return;
    }

    if(fd_mode[fd] != 'a')
    {
        cout<<BOLDRED<<"FILE IS NOT IN APPEND MODE"<<RESET<<endl;
        return;
    }

    string filename = fd_to_filename[fd];
    int inum = filename_to_inode[filename];
    int it = current_inode_arr[inum].lastblock();

    cout<<endl<<BOLDGREEN<<"FILE CONTENTS CURRENTLY ARE:"<<RESET<<endl;

    string res = read_helper(current_disk,current_inode_arr[inum].pointer[it]*BLOCK_SIZE);

    cout<<endl;
    cout<<BOLDYELLOW<<"ENTER FILE CONTENT YOU WANT TO APPEND :"<<RESET<<endl;

    string content="", temp="";
    while(true){
        content += "\n";
        getline(cin>>ws, temp);
        if(temp == ":q")
            break;
        content += temp;
    }


    current_inode_arr[inum].clearblock(it);

    string buffer = res+" "+content;

    current_inode_arr[inum].filesize += content.length();
    for(int i = it; i<10;i++) //change
    {

        if(buffer.length()<i*BLOCK_SIZE)
        {
            continue;
        }

        if(i>it)
        {
            current_sb.free_blocks_map[current_sb.last_block]='1';
            current_sb.free_blocks--;

            current_inode_arr[inum].pointer[i]= current_sb.last_block++;
        }


        string tempcontent= buffer.substr(i*BLOCK_SIZE,BLOCK_SIZE);

        write_helper(current_disk,tempcontent,current_inode_arr[inum].pointer[i]*BLOCK_SIZE);


    }

    cout<<BOLDGREEN<<"APPEND SUCCESSFUL"<<RESET<<endl;


}

//------------------------------------------------------------------------------------------------
void read_file(string fd)
{
    if(fd_to_filename.find(fd) == fd_to_filename.end())
    {
        cout<<BOLDRED<<"INVALID FILE DESCRIPTOR SELECTED"<<RESET<<endl;
        return;
    }

    if(fd_mode[fd] != 'r')
    {
        cout<<BOLDRED<<"FILE IS NOT IN READ MODE"<<RESET<<endl;
        return;
    }

    string filename = fd_to_filename[fd];
    int inum = filename_to_inode[filename];
    inode curfile = current_inode_arr[inum];

    cout<<BOLDGREEN<<"FILE CONTENTS ARE :"<<RESET<<endl;

    for(int i=0;i<10;i++)
    {
        if(curfile.pointer[i] != -1)
        {
            read_helper(current_disk,curfile.pointer[i]*BLOCK_SIZE);

        }
    }

    cout<<endl;
}


void close_file(string fd)
{
    if(fd_to_filename.find(fd) == fd_to_filename.end())
    {
        cout<<BOLDRED<<"INVALID FILE DESCRIPTOR SELECTED"<<RESET<<endl;
        return;
    }

    opened_fd.erase(fd);
    string filename = fd_to_filename[fd];
    opened_files.erase(filename);
    fd_mode.erase(fd);

    cout<<endl<<BOLDGREEN<<"FILE CLOSE SUCCESSFUL"<<RESET<<endl;


}


void delete_file(string filename)
{

    if(filename_to_inode.find(filename) == filename_to_inode.end())
    {
        cout<<BOLDYELLOW<<filename<<BOLDRED<<" DOESN'T EXIST IN THE DISK"<<RESET<<endl;
    }

    if(opened_files.find(filename) != opened_files.end())
    {
        cout<<BOLDRED<<"OPENED FILE CANNOT BE DELETED"<<RESET<<endl;
        return;
    }

    int inum = filename_to_inode[filename];
    filename_to_inode.erase(filename);

    strcpy(current_inode_arr[inum].filename,nullfile);
    current_inode_arr[inum].clearinode();

    cout<<BOLDYELLOW<<filename<<BOLDGREEN<<" SUCCESSFULLY DELETED"<<RESET<<endl;

}


void list_files()
{
    int i=1;

    cout<<BOLDGREEN<<"FILES PRESENT:"<<BOLDYELLOW<<endl;
    for(auto it=filename_to_inode.begin(); it!=filename_to_inode.end(); it++)
    {
        cout<<i<<". " << it->first <<endl;
        i++;
    }

    cout<<endl;
}


void list_opened_files()
{
    int i=1;

    cout<<BOLDGREEN<<"\tFILENAME\tDESCRIPTOR\tMODE"<<BOLDYELLOW<<endl;
    for(auto it=fd_to_filename.begin(); it!= fd_to_filename.end(); it++)
    {
        cout<<i<<".\t"<<it->second<<"\t"<<it->first<<"\t\t"<<fd_mode[it->first]<<endl;
        i++;
    }

    cout<<RESET;
}


//----------------------------------------------------------------------------------------------------------------------

void disk_operations()
{
    int ch;
    while(1) {
        clearScreen();
        
        cout <<BOLDCYAN<< "1.Create File\n2.Open File\n3.Read File\n4.Write File\n5.Append File\n6.Close File\n7.Delete File\n8.List Files\n9.List Opened Files\n10. Unmount\n"<<BOLDYELLOW<<">>"<<RESET;
        cin >> ch;



        if(ch == 1)
        {
            cout<<BOLDYELLOW<<"ENTER NAME OF THE FILE : "<<RESET<<endl;
            string temp;
            getline(cin>>ws,temp);
            create_file(temp);
        }
        else if(ch == 2)
        {
            cout<<BOLDYELLOW<<"ENTER NAME OF THE FILE : "<<RESET;
            string temp;
            getline(cin>>ws,temp);
            open_file(temp);
        }
        else if(ch == 3)
        {
            cout<<BOLDYELLOW<<"ENTER FILE DESCRIPTOR : "<<RESET;
            string temp;
            getline(cin>>ws,temp);
            read_file(temp);
        }
        else if(ch == 4)
        {
            cout<<BOLDYELLOW<<"ENTER FILE DESCRIPTOR : "<<RESET;
            string temp;
            getline(cin>>ws,temp);
            write_file(temp);
        }
        else if(ch == 5)
        {
            cout<<BOLDYELLOW<<"ENTER FILE DESCRIPTOR : "<<RESET;
            string temp;
            getline(cin>>ws,temp);
            append_file(temp);
        }
        else if(ch == 6)
        {
            cout<<BOLDYELLOW<<"ENTER FILE DESCRIPTOR : "<<RESET;
            string temp;
            getline(cin>>ws,temp);
            close_file(temp);
        }
        else if(ch == 7)
        {
            cout<<BOLDYELLOW<<"ENTER NAME OF THE FILE : "<<RESET;
            string temp;
            getline(cin>>ws,temp);
            delete_file(temp);
        }
        else if(ch == 8)
        {
            list_files();
        }
        else if(ch == 9)
        {
            list_opened_files();
        }
        else if(ch == 10)
        {
            dismount_disk();
            break;
        }
        else
        {
            cout<<BOLDRED<<"INVALID CHOICE!!"<<RESET<<endl;
        }

        cout<<endl<<BOLDYELLOW<<"PRESS ENTER TO CONTINUE"<<RESET<<endl;
        char c;
        cin>>c;
    }
}


int main()
{
    cout<<current_sb.pointer_to_blocks<<endl;
    int ch1;
    string filename;
    while(1)
    {
        clearScreen();
        cout<<BOLDCYAN<<"1.Create Disk\n2.Mount Disk\n3.Exit\n"<<BOLDYELLOW<<">>"<<RESET;
        cin>>ch1;


        if(ch1==1)
        {

            cout<<BOLDYELLOW<<"ENTER DISK NAME : "<<RESET;

            getline(cin>>ws,filename);
            create_disk(filename);
        }
        else if(ch1 == 2)
        {
            cout<<BOLDYELLOW<<"ENTER DISK NAME : "<<RESET;
            getline(cin>>ws,filename);
            mount_disk(filename);
            disk_operations();
        }
        else if(ch1==3)
        {
            dismount_disk();
            break;
        }
        else
        {
            cout<<BOLDRED<<"INVALID CHOICE"<<RESET<<endl;

        }


    }

    clearScreen();

    return 0;
}
