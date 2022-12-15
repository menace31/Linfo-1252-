#include "lib_tar.h"

/**
 * Checks whether the archive is valid.
 *
 * Each non-null header of a valid archive has:
 *  - a magic value of "ustar" and a null,
 *  - a version value of "00" and no null,
 *  - a correct checksum
 *
 * @param tar_fd A file descriptor pointing to the start of a file supposed to contain a tar archive.
 *
 * @return a zero or positive value if the archive is valid, representing the number of non-null headers in the archive,
 *         -1 if the archive contains a header with an invalid magic value,
 *         -2 if the archive contains a header with an invalid version value,
 *         -3 if the archive contains a header with an invalid checksum value
 */



int check_archive(int tar_fd) {
    tar_header_t *header = (tar_header_t *)malloc(SizeOfHeader);
    int count =0;
    int offset = 0;
    int sum;
    while(true){
        pread(tar_fd, header, SizeOfHeader, offset);
        if (!header->name[0])
            break;
        count++;
        if (header->typeflag == DIRTYPE)
        {
            offset += SizeOfHeader;
        }
        else
        {
            offset += (SizeOfHeader*2 - (TAR_INT(header->size) % SizeOfHeader)) + TAR_INT(header->size);
        }
        if (strncmp(TMAGIC, header->magic, TMAGLEN) != 0){
            free(header);
            return -1;
        }

        if (strncmp(TVERSION, header->version, TVERSLEN) != 0){
            free(header);
            return -2;
        }

        sum = 0;
        char *header_list = (char *)header;
        for (int i = 0; i < SizeOfHeader; i++){
            if (i < 156 && i>= 148){
                sum +=32;
            }
            else {
                sum += header_list[i];
            }
        }
        if (TAR_INT(header->chksum) != sum){
            free(header);
            return -3;
        }
    }
    free(header);
    return count;
}

/**
 * Checks whether an entry exists in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive,
 *         any other value otherwise.
 */
int exists(int tar_fd, char *path)
{
    int offset = 0;
    tar_header_t *header = (tar_header_t *)malloc(SizeOfHeader);

    while(true)
    {
        pread(tar_fd, header, SizeOfHeader, offset);
        if (!header->name[0])
            break;

        if (!strcmp(header->name, path))
        {
            free(header);
            return 1;
        }
        if (header->typeflag == DIRTYPE)
        {
            offset += SizeOfHeader;
        }
        else
        {
            offset += (SizeOfHeader*2 - (TAR_INT(header->size) % SizeOfHeader)) + TAR_INT(header->size);
        }
    }
    free(header);
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a directory.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a directory,
 *         any other value otherwise.
 */
int is_dir(int tar_fd, char *path) {

    tar_header_t *header = (tar_header_t *)malloc(SizeOfHeader);

    int offset = 0;
    while(true){
        pread(tar_fd, header, SizeOfHeader, offset);
        if (!header->name[0])
            break;
        if (header->typeflag == DIRTYPE)
        {
            offset += SizeOfHeader;
        }
        else
        {
            offset += (SizeOfHeader*2 - (TAR_INT(header->size) % SizeOfHeader)) + TAR_INT(header->size);
        }
        if (!strcmp(header->name, path)){
            if (header->typeflag == DIRTYPE){
                free(header);
                return 1;
            }
            else {
                free(header);
                return 0;
            }
        }
    }
    free(header);
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a file.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a file,
 *         any other value otherwise.
 */
int is_file(int tar_fd, char *path) {

    tar_header_t *header = (tar_header_t *)malloc(SizeOfHeader);

    int offset = 0;
    while(true)
    {
        pread(tar_fd, header, SizeOfHeader, offset);
        if (!header->name[0])
            break;


        if (strcmp(header->name, path) == 0){
            if (header->typeflag == REGTYPE || header->typeflag == AREGTYPE){
                free(header);
                return 1;
            }
            else {
                free(header);
                return 0;
            }
        }
        if (header->typeflag == DIRTYPE)
        {
            offset += SizeOfHeader;
        }
        else
        {
            offset += (SizeOfHeader*2 - (TAR_INT(header->size) % SizeOfHeader)) + TAR_INT(header->size);
        }
        
    }
    free(header);
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a symlink.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 * @return zero if no entry at the given path exists in the archive or the entry is not symlink,
 *         any other value otherwise.
 */
int is_symlink(int tar_fd, char *path) {

    tar_header_t *header = (tar_header_t *)malloc(SizeOfHeader);

    int offset = 0;

    while(true){
        pread(tar_fd, header, SizeOfHeader, offset);
        if (!header->name[0])
            break;
        if (header->typeflag == DIRTYPE)
        {
            offset += SizeOfHeader;
        }
        else
        {
            offset += (SizeOfHeader*2 - (TAR_INT(header->size) % SizeOfHeader)) + TAR_INT(header->size);
        }  
        

        if (strcmp(header->name, path) == 0){
            if (header->typeflag == LNKTYPE || header->typeflag == SYMTYPE){
                free(header);
                return 1;
            }
            else {
                free(header);
                return 0;
            } 
        }

    }
    free(header);
    return 0;
}


/**
 * Lists the entries at a given path in the archive.
 * list() does not recurse into the directories listed at the given path.
 *
 * Example:
 *  dir/          list(..., "dir/", ...) lists "dir/a", "dir/b", "dir/c/" and "dir/e/"
 *   ├── a
 *   ├── b
 *   ├── c/
 *   │   └── d
 *   └── e/
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive. If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param entries An array of char arrays, each one is long enough to contain a tar entry path.
 * @param no_entries An in-out argument.
 *                   The caller set it to the number of entries in `entries`.
 *                   The callee set it to the number of entries listed.
 *
 * @return zero if no directory at the given path exists in the archive,
 *         any other value otherwise.
 */
int list(int tar_fd, char *path, char **entries, size_t *no_entries) {
    
    size_t entries_number = 0;
    int path_len = strlen(path)-1;
    int offset = 0;

    tar_header_t *header = (tar_header_t *)malloc(SizeOfHeader);

    if(is_dir(tar_fd,path) == 0 && is_symlink(tar_fd,path) == 0){
        *no_entries = 0;
        free(header);
        return 0;
    }

    while(true){
        pread(tar_fd, header, SizeOfHeader, offset);
        if (!header->name[0]){
            break;
        }
        if (strcmp(header->name, path) == 0){
            if (header->typeflag == SYMTYPE || header->typeflag == LNKTYPE){
                int ltar = list(tar_fd, header->linkname, entries, no_entries);
                free(header);
                return ltar;
            }
        }
        else if (strncmp(header->name, path, path_len) == 0){
            if (*no_entries <= entries_number){
                *no_entries = entries_number;
                free(header);
                return entries_number;
            }
            else{
                strcpy(entries[entries_number], header->name);
                entries_number++;
            }
        }
        if (header->typeflag == DIRTYPE)
        {
            offset += SizeOfHeader;
        }
        else
        {
            offset += (SizeOfHeader*2 - (TAR_INT(header->size) % SizeOfHeader)) + TAR_INT(header->size);
        }
    }
    *no_entries = entries_number;
    free(header);
    return entries_number;


}

/**
 * Reads a file at a given path in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive to read from.  If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param offset An offset in the file from which to start reading from, zero indicates the start of the file.
 * @param dest A destination buffer to read the given file into.
 * @param len An in-out argument.
 *            The caller set it to the size of dest.
 *            The callee set it to the number of bytes written to dest.
 *
 * @return -1 if no entry at the given path exists in the archive or the entry is not a file,
 *         -2 if the offset is outside the file total length,
 *         zero if the file was read in its entirety into the destination buffer,
 *         a positive value if the file was partially read, representing the remaining bytes left to be read to reach
 *         the end of the file.
 *
 */

ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len) {
    tar_header_t *header = (tar_header_t *)malloc(SizeOfHeader);
    int head_offset = 0;
    if (!is_file(tar_fd, path))
    {
        free(header);
        return -1;
    }
    

    struct stat stats;
    fstat(tar_fd, &stats);

    while (head_offset + SizeOfHeader < stats.st_size)
    {
        pread(tar_fd, header, SizeOfHeader, head_offset);
        if (!strcmp(path,header->name))
        {
            int value;
            if (header->typeflag == LNKTYPE || header->typeflag == SYMTYPE)
            {
                value = read_file(tar_fd, header->linkname, offset, dest, len);
            }
            else if (offset > TAR_INT(header->size))
            {
                value = -2;
            }
            else
            {
                head_offset += SizeOfHeader + offset;
                int to_read = *len;
                value = TAR_INT(header->size) - *len - offset;

                if (TAR_INT(header->size) <= *len + offset)
                {
                    to_read = TAR_INT(header->size);
                    *len = TAR_INT(header->size) - offset;
                    value = 0;
                }
                pread(tar_fd, dest, to_read, head_offset);
            }
            
            free(header);
            return value;
        }
        
        if (!TAR_INT(header->size))
            head_offset += SizeOfHeader;
        else
            head_offset += TAR_INT(header->size) + 2*SizeOfHeader - (TAR_INT(header->size) % SizeOfHeader);
    }
    free(header);
    return head_offset - stats.st_size;
    
}