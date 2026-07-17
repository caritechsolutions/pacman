//--------------------------------------------------------------------------
// Program to pull the information out of various types of EXIF digital
// camera files and show it in a reasonably consistent way
//
// Version 2.90
//
// Compiling under Windows:
//   Make sure you have Microsoft's compiler on the path, then run make.bat
//
// Dec 1999 - Feb 2010
//
// by Matthias Wandel   www.sentex.net/~mwandel
//--------------------------------------------------------------------------
#include "jhead.h"
#include "gui_core.h"
#include "gxcore.h"

#include <sys/stat.h>

#define JHEAD_VERSION "2.90"

// This #define turns on features that are too very specific to
// how I organize my photos.  Best to ignore everything inside #ifdef MATTHIAS
//#define MATTHIAS

#ifdef _WIN32
    #include <io.h>
#endif

static int FilesMatched;
static int FileSequence;

static const char * CurrentFile;

//--------------------------------------------------------------------------
// Command line options flags
static int TrimExif = FALSE;        // Cut off exif beyond interesting data.
static int RenameToDate = 0;        // 1=rename, 2=rename all.
#ifdef _WIN32
static int RenameAssociatedFiles = FALSE;
#endif
static char * strftime_args = NULL; // Format for new file name.
static int Exif2FileTime  = FALSE;
static int DoModify     = FALSE;
static int DoReadAction = FALSE;
       int ShowTags     = FALSE;    // Do not show raw by default.
static int Quiet        = FALSE;    // Be quiet on success (like unix programs)
       int DumpExifMap  = FALSE;
static int ShowConcise  = FALSE;
static int CreateExifSection = FALSE;
static char * ApplyCommand = NULL;  // Apply this command to all images.
static char * FilterModel = NULL;
static int    ExifOnly    = FALSE;
static int    PortraitOnly = FALSE;
static time_t ExifTimeAdjust = 0;   // Timezone adjust
static time_t ExifTimeSet = 0;      // Set exif time to a value.
static char DateSet[11];
static unsigned DateSetChars = 0;
static unsigned FileTimeToExif = FALSE;

static int DeleteComments = FALSE;
static int DeleteExif = FALSE;
static int DeleteIptc = FALSE;
static int DeleteXmp = FALSE;
static int DeleteUnknown = FALSE;
static char * ThumbSaveName = NULL; // If not NULL, use this string to make up
                                    // the filename to store the thumbnail to.

static char * ThumbInsertName = NULL; // If not NULL, use this string to make up
                                    // the filename to retrieve the thumbnail from.

static int RegenThumbnail = FALSE;

static char * ExifXferScrFile = NULL;// Extract Exif header from this file, and
                                    // put it into the Jpegs processed.

static int EditComment = FALSE;     // Invoke an editor for editing the comment
static int SupressNonFatalErrors = FALSE; // Wether or not to pint warnings on recoverable errors

static char * CommentSavefileName = NULL; // Save comment to this file.
static char * CommentInsertfileName = NULL; // Insert comment from this file.
static char * CommentInsertLiteral = NULL;  // Insert this comment (from command line)

static int AutoRotate = FALSE;
static int ZeroRotateTagOnly = FALSE;

                                    // (file name, file size, file date)


#ifdef MATTHIAS
    // This #ifdef to take out less than elegant stuff for editing
    // the comments in a JPEG.  The programs rdjpgcom and wrjpgcom
    // included with Linux distributions do a better job.

    static char * AddComment = NULL; // Add this tag.
    static char * RemComment = NULL; // Remove this tag
    static int AutoResize = FALSE;
#endif // MATTHIAS

//--------------------------------------------------------------------------
// Error exit handler
//--------------------------------------------------------------------------
void ErrFatal(char * msg)
{
    gxlogf ("\nError : %s\n", msg);
    if (CurrentFile) gxlogf("in file '%s'\n",CurrentFile);
    //exit(EXIT_FAILURE);
}

//--------------------------------------------------------------------------
// Report non fatal errors.  Now that microsoft.net modifies exif headers,
// there's corrupted ones, and there could be more in the future.
//--------------------------------------------------------------------------
void ErrNonfatal(char * msg, int a1, int a2)
{
    if (SupressNonFatalErrors) return;

    gxlogf("\nNonfatal Error : ");
    if (CurrentFile) gxlogf("'%s' ",CurrentFile);
    gxlogf( msg, a1, a2);
    gxlogf( "\n");
}


#ifdef GUI_JPEG_INFO
//--------------------------------------------------------------------------
// Invoke an editor for editing a string.
//--------------------------------------------------------------------------
static int FileEditComment(char * TempFileName, char * Comment, int CommentSize)
{
    FILE * file;
    int a;
    char QuotedPath[JPEG_PATH_MAX+10];

    file = fopen(TempFileName, "w");
    if (file == NULL){
        gxlogf( "Can't create file '%s'\n",TempFileName);
        ErrFatal("could not create temporary file");
    }
    fwrite(Comment, CommentSize, 1, file);

    fclose(file);

    fflush(stdout); // So logs are contiguous.

    {
        char * Editor;
        Editor = getenv("EDITOR");
        if (Editor == NULL){
#ifdef _WIN32
            Editor = "notepad";
#else
            Editor = "vi";
#endif
        }
        if (strlen(Editor) > JPEG_PATH_MAX) ErrFatal("env too long");

        sprintf(QuotedPath, "%s \"%s\"",Editor, TempFileName);
        a = system(QuotedPath);
    }

    if (a != 0){
        perror("Editor failed to launch");
        exit(-1);
    }

    file = fopen(TempFileName, "r");
    if (file == NULL){
        ErrFatal("could not open temp file for read");
    }

    // Read the file back in.
    CommentSize = fread(Comment, 1, 999, file);

    fclose(file);

    unlink(TempFileName);

    return CommentSize;
}
#endif/*GUI_JPEG_INFO*/

#ifdef MATTHIAS
//--------------------------------------------------------------------------
// Modify one of the lines in the comment field.
// This very specific to the photo album program stuff.
//--------------------------------------------------------------------------
static char KnownTags[][10] = {"date", "desc","scan_date","author",
                               "keyword","videograb",
                               "show_raw","panorama","titlepix",""};

static int ModifyDescriptComment(char * OutComment, char * SrcComment)
{
    char Line[500];
    int Len;
    int a,i;
    unsigned l;
    int HasScandate = FALSE;
    int TagExists = FALSE;
    int Modified = FALSE;
    Len = 0;

    OutComment[0] = 0;


    for (i=0;;i++){
        if (SrcComment[i] == '\r' || SrcComment[i] == '\n' || SrcComment[i] == 0 || Len >= 199){
            // Process the line.
            if (Len > 0){
                Line[Len] = 0;
                //gxlogd("Line: '%s'\n",Line);
                for (a=0;;a++){
                    l = strlen(KnownTags[a]);
                    if (!l){
                        // Unknown tag.  Discard it.
                        gxlogd("Error: Unknown tag '%s'\n", Line); // Deletes the tag.
                        Modified = TRUE;
                        break;
                    }
                    if (memcmp(Line, KnownTags[a], l) == 0){
                        if (Line[l] == ' ' || Line[l] == '=' || Line[l] == 0){
                            // Its a good tag.
                            if (Line[l] == ' ') Line[l] = '='; // Use equal sign for clarity.
                            if (a == 2) break; // Delete 'orig_path' tag.
                            if (a == 3) HasScandate = TRUE;
                            if (RemComment){
                                if (strlen(RemComment) == l){
                                    if (!memcmp(Line, RemComment, l)){
                                        Modified = TRUE;
                                        break;
                                    }
                                }
                            }
                            if (AddComment){
                                // Overwrite old comment of same tag with new one.
                                if (!memcmp(Line, AddComment, l+1)){
                                    TagExists = TRUE;
                                    strncpy(Line, AddComment, sizeof(Line));
                                    Modified = TRUE;
                                }
                            }
                            strncat(OutComment, Line, MAX_COMMENT_SIZE-5-strlen(OutComment));
                            strcat(OutComment, "\n");
                            break;
                        }
                    }
                }
            }
            Line[Len = 0] = 0;
            if (SrcComment[i] == 0) break;
        }else{
            Line[Len++] = SrcComment[i];
        }
    }

    if (AddComment && TagExists == FALSE){
        strncat(OutComment, AddComment, MAX_COMMENT_SIZE-5-strlen(OutComment));
        strcat(OutComment, "\n");
        Modified = TRUE;
    }

    if (!HasScandate && !ImageInfo.DateTime[0]){
        // Scan date is not in the file yet, and it doesn't have one built in.  Add it.
        char Temp[30];
        sprintf(Temp, "scan_date=%s", ctime(&ImageInfo.FileDateTime));
        strncat(OutComment, Temp, MAX_COMMENT_SIZE-5-strlen(OutComment));
        Modified = TRUE;
    }
    return Modified;
}
//--------------------------------------------------------------------------
// Automatic make smaller command stuff
//--------------------------------------------------------------------------
static int AutoResizeCmdStuff(void)
{
    static char CommandString[JPEG_PATH_MAX+1];
    double scale;

    ApplyCommand = CommandString;

    if (ImageInfo.Height <= 1280 && ImageInfo.Width <= 1280){
        gxlogd("not resizing %dx%x '%s'\n",ImageInfo.Height, ImageInfo.Width, ImageInfo.FileName);
        return FALSE;
    }

    scale = 1024.0 / ImageInfo.Height;
    if (1024.0 / ImageInfo.Width < scale) scale = 1024.0 / ImageInfo.Width;

    if (scale < 0.5) scale = 0.5; // Don't scale down by more than a factor of two.

    sprintf(CommandString, "mogrify -geometry %dx%d -quality 85 &i",(int)(ImageInfo.Width*scale), (int)(ImageInfo.Height*scale));
    return TRUE;
}


#endif // MATTHIAS


//--------------------------------------------------------------------------
// check if this file should be skipped based on contents.
//--------------------------------------------------------------------------
#ifdef GUI_JPEG_INFO
static int CheckFileSkip(void)
{
    // I sometimes add code here to only process images based on certain
    // criteria - for example, only to convert non progressive Jpegs to progressives, etc..

    if (FilterModel){
        // Filtering processing by camera model.
        // This feature is useful when pictures from multiple cameras are colated,
        // the its found that one of the cameras has the time set incorrectly.
        if (strstr(ImageInfo.CameraModel, FilterModel) == NULL){
            // Skip.
            return TRUE;
        }
    }

    if (ExifOnly){
        // Filtering by EXIF only.  Skip all files that have no Exif.
        if (FindSection(M_EXIF) == NULL){
            return TRUE;
        }
    }

    if (PortraitOnly == 1){
        if (ImageInfo.Width > ImageInfo.Height) return TRUE;
    }

    if (PortraitOnly == -1){
        if (ImageInfo.Width < ImageInfo.Height) return TRUE;
    }

    return FALSE;
}

//--------------------------------------------------------------------------
// Subsititute original name for '&i' if present in specified name.
// This to allow specifying relative names when manipulating multiple files.
//--------------------------------------------------------------------------
static void RelativeName(char * OutFileName, const char * NamePattern, const char * OrigName)
{
    // If the filename contains substring "&i", then substitute the
    // filename for that.  This gives flexibility in terms of processing
    // multiple files at a time.
    char * Subst;
    Subst = strstr(NamePattern, "&i");
    if (Subst){
        strncpy(OutFileName, NamePattern, Subst-NamePattern);
        OutFileName[Subst-NamePattern] = 0;
        strncat(OutFileName, OrigName, JPEG_PATH_MAX);
        strncat(OutFileName, Subst+2, JPEG_PATH_MAX);
    }else{
        strncpy(OutFileName, NamePattern, JPEG_PATH_MAX);
    }
}

#endif /*GUI_JPEG_INFO*/

#ifdef _WIN32
//--------------------------------------------------------------------------
// Rename associated files
//--------------------------------------------------------------------------
void RenameAssociated(const char * FileName, char * NewBaseName)
{
    int a;
    int PathLen;
    int ExtPos;
    char FilePattern[_MAX_PATH+1];
    char NewName[_MAX_PATH+1];
    struct _finddata_t finddata;
    long find_handle;

    for(ExtPos = strlen(FileName);FileName[ExtPos-1] != '.';){
        if (--ExtPos == 0) return; // No extension!
    }

    memcpy(FilePattern, FileName, ExtPos);
    FilePattern[ExtPos] = '*';
    FilePattern[ExtPos+1] = '\0';

    for(PathLen = strlen(FileName);FileName[PathLen-1] != SLASH;){
        if (--PathLen == 0) break;
    }

    find_handle = _findfirst(FilePattern, &finddata);

    for (;;){
        if (find_handle == -1) break;

        // Eliminate the obvious patterns.
        if (!memcmp(finddata.name, ".",2)) goto next_file;
        if (!memcmp(finddata.name, "..",3)) goto next_file;
        if (finddata.attrib & _A_SUBDIR) goto next_file;

        strncpy(FilePattern+PathLen, finddata.name, JPEG_PATH_MAX-PathLen); // full name with path

        strcpy(NewName, NewBaseName);
        for(a = strlen(finddata.name);finddata.name[a] != '.';){
            if (--a == 0) goto next_file;
        }
        strncat(NewName, finddata.name+a, _MAX_PATH-strlen(NewName)); // add extension to new name

        if (rename(FilePattern, NewName) == 0){
            if (!Quiet){
                gxlogd("%s --> %s\n",FilePattern, NewName);
            }
        }

        next_file:
        if (_findnext(find_handle, &finddata) != 0) break;
    }
    _findclose(find_handle);
}
#endif

//--------------------------------------------------------------------------
// Handle renaming of files by date.
//--------------------------------------------------------------------------
#ifdef GUI_JPEG_INFO
static void DoFileRenaming(const char * FileName)
{
    int NumAlpha = 0;
    int NumDigit = 0;
    int PrefixPart = 0; // Where the actual filename starts.
    int ExtensionPart;  // Where the file extension starts.
    int a;
    struct tm tm;
    char NewBaseName[JPEG_PATH_MAX*2];
    int AddLetter = 0;
    char NewName[JPEG_PATH_MAX+2];

    ExtensionPart = strlen(FileName);
    for (a=0;FileName[a];a++){
        if (FileName[a] == SLASH){
            // Don't count path component.
            NumAlpha = 0;
            NumDigit = 0;
            PrefixPart = a+1;
        }

        if (FileName[a] == '.') ExtensionPart = a;  // Remember where extension starts.

        if (isalpha(FileName[a])) NumAlpha += 1;    // Tally up alpha vs. digits to judge wether to rename.
        if (isdigit(FileName[a])) NumDigit += 1;
    }

    if (RenameToDate <= 1){
        // If naming isn't forced, ensure name is mostly digits, or leave it alone.
        if (NumAlpha > 8 || NumDigit < 4){
            return;
        }
    }

    if (!Exif2tm(&tm, ImageInfo.DateTime)){
        gxlogd("File '%s' contains no exif date stamp.  Using file date\n",FileName);
        // Use file date/time instead.
        tm = *localtime(&ImageInfo.FileDateTime);
    }


    strncpy(NewBaseName, FileName, JPEG_PATH_MAX); // Get path component of name.

    if (strftime_args){
        // Complicated scheme for flexibility.  Just pass the args to strftime.
        time_t UnixTime;

        char *s;
        char pattern[JPEG_PATH_MAX+20];
        int n = ExtensionPart - PrefixPart;

        // Call mktime to get weekday and such filled in.
        UnixTime = mktime(&tm);
        if ((int)UnixTime == -1){
            gxlogd("Could not convert %s to unix time",ImageInfo.DateTime);
            return;
        }

        // Substitute "%f" for the original name (minus path & extension)
        pattern[JPEG_PATH_MAX-1]=0;
        strncpy(pattern, strftime_args, JPEG_PATH_MAX-1);
        while ((s = strstr(pattern, "%f")) && strlen(pattern) + n < JPEG_PATH_MAX-1){
            memmove(s + n, s + 2, strlen(s+2) + 1);
            memmove(s, FileName + PrefixPart, n);
        }

        {
            // Sequential number renaming part.
            // '%i' type pattern becomes sequence number.
            int ppos = -1;
            for (a=0;pattern[a];a++){
                if (pattern[a] == '%'){
                     ppos = a;
                }else if (pattern[a] == 'i'){
                    if (ppos >= 0 && a<ppos+4){
                        // Replace this part with a number.
                        char pat[8], num[16];
                        int l,nl;
                        memcpy(pat, pattern+ppos, 4);
                        pat[a-ppos] = 'd'; // Replace 'i' with 'd' for '%d'
                        pat[a-ppos+1] = '\0';
                        sprintf(num, pat, FileSequence); // let gxlogd do the number formatting.
                        nl = strlen(num);
                        l = strlen(pattern+a+1);
                        if (ppos+nl+l+1 >= JPEG_PATH_MAX) ErrFatal("str overflow");
                        memmove(pattern+ppos+nl, pattern+a+1, l+1);
                        memcpy(pattern+ppos, num, nl);
                        break;
                    }
                }else if (!isdigit(pattern[a])){
                    ppos = -1;
                }
            }
        }
        strftime(NewName, JPEG_PATH_MAX, pattern, &tm);
    }else{
        // My favourite scheme.
        sprintf(NewName, "%02d%02d-%02d%02d%02d",
             tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

    NewBaseName[PrefixPart] = 0;
    CatPath(NewBaseName, NewName);

    AddLetter = isdigit(NewBaseName[strlen(NewBaseName)-1]);
    for (a=0;;a++){
        char NewName[JPEG_PATH_MAX+10];
        char NameExtra[3];
        struct stat dummy;

        if (a){
            // Generate a suffix for the file name if previous choice of names is taken.
            // depending on wether the name ends in a letter or digit, pick the opposite to separate
            // it.  This to avoid using a separator character - this because any good separator
            // is before the '.' in ascii, and so sorting the names would put the later name before
            // the name without suffix, causing the pictures to more likely be out of order.
            if (AddLetter){
                NameExtra[0] = (char)('a'-1+a); // Try a,b,c,d... for suffix if it ends in a number.
            }else{
                NameExtra[0] = (char)('0'-1+a); // Try 0,1,2,3... for suffix if it ends in a latter.
            }
            NameExtra[1] = 0;
        }else{
            NameExtra[0] = 0;
        }

        sprintf(NewName, "%s%s.jpg", NewBaseName, NameExtra);

        if (!strcmp(FileName, NewName)) break; // Skip if its already this name.

        if (!EnsurePathExists(NewBaseName)){
            break;
        }


        if (stat(NewName, &dummy)){
            // This name does not pre-exist.
            if (rename(FileName, NewName) == 0){
                gxlogd("%s --> %s\n",FileName, NewName);
#ifdef _WIN32
                if (RenameAssociatedFiles){
                    sprintf(NewName, "%s%s", NewBaseName, NameExtra);
                    RenameAssociated(FileName, NewName);
                }
#endif
            }else{
                gxlogd("Error: Couldn't rename '%s' to '%s'\n",FileName, NewName);
            }
            break;

        }

        if (a > 25 || (!AddLetter && a > 9)){
            gxlogd("Possible new names for for '%s' already exist\n",FileName);
            break;
        }
    }
}

//--------------------------------------------------------------------------
// Rotate the image and its thumbnail
//--------------------------------------------------------------------------
static int DoAutoRotate(const char * FileName)
{
    if (ImageInfo.Orientation >= 2 && ImageInfo.Orientation <= 8){
        const char * Argument;
        Argument = ClearOrientation();

        if (!ZeroRotateTagOnly){
            char RotateCommand[JPEG_PATH_MAX*2+50];
            if (Argument == NULL){
                ErrFatal("Orientation screwup");
            }

            sprintf(RotateCommand, "jpegtran -trim -%s -outfile &o &i", Argument);
            ApplyCommand = RotateCommand;
            //DoCommand(FileName, FALSE);
            ApplyCommand = NULL;

            // Now rotate the thumbnail, if there is one.
            if (ImageInfo.ThumbnailOffset &&
                ImageInfo.ThumbnailSize &&
                ImageInfo.ThumbnailAtEnd){
                // Must have a thumbnail that exists and is modifieable.

                char ThumbTempName_in[JPEG_PATH_MAX+5];
                char ThumbTempName_out[JPEG_PATH_MAX+5];

                strcpy(ThumbTempName_in, FileName);
                strcat(ThumbTempName_in, ".thi");
                strcpy(ThumbTempName_out, FileName);
                strcat(ThumbTempName_out, ".tho");
                SaveThumbnail(ThumbTempName_in);
                sprintf(RotateCommand,"jpegtran -trim -%s -outfile \"%s\" \"%s\"",
                    Argument, ThumbTempName_out, ThumbTempName_in);

                if (system(RotateCommand) == 0){
                    // Put the thumbnail back in the header
                    ReplaceThumbnail(ThumbTempName_out);
                }

                unlink(ThumbTempName_in);
                unlink(ThumbTempName_out);
            }
        }
        return TRUE;
    }
    return FALSE;
}

//--------------------------------------------------------------------------
// Regenerate the thumbnail using mogrify
//--------------------------------------------------------------------------
static int RegenerateThumbnail(const char * FileName)
{
    char ThumbnailGenCommand[JPEG_PATH_MAX*2+50];
    if (ImageInfo.ThumbnailOffset == 0 || ImageInfo.ThumbnailAtEnd == FALSE){
        // There is no thumbnail, or the thumbnail is not at the end.
        return FALSE;
    }

    sprintf(ThumbnailGenCommand, "mogrify -thumbnail %dx%d \"%s\"",
        RegenThumbnail, RegenThumbnail, FileName);

    if (system(ThumbnailGenCommand) == 0){
        // Put the thumbnail back in the header
        return ReplaceThumbnail(FileName);
    }else{
        ErrFatal("Unable to run 'mogrify' command");
        return FALSE;
    }
    return (0);
}
#endif/*GUI_JPEG_INFO*/

//--------------------------------------------------------------------------
// Set file time as exif time.
//--------------------------------------------------------------------------
void FileTimeAsString(char * TimeStr)
{
#ifdef GUI_JPEG_INFO
    struct tm ts;
    ts = *localtime(&ImageInfo.FileDateTime);
    strftime(TimeStr, 20, "%Y:%m:%d %H:%M:%S", &ts);
#endif/*GUI_JPEG_INFO*/
}

//--------------------------------------------------------------------------
// Do selected operations to one file at a time.
//--------------------------------------------------------------------------
void ProcessFile(const char * FileName)
{
#ifdef GUI_JPEG_INFO
    int Modified = FALSE;
    ReadMode_t ReadMode;

    if (strlen(FileName) >= JPEG_PATH_MAX-1){
        // Protect against buffer overruns in strcpy / strcat's on filename
	return;
        ErrFatal("filename too long");
    }

    ReadMode = READ_METADATA;
    CurrentFile = FileName;
    FilesMatched = 1;

    ResetJpgfile();

    // Start with an empty image information structure.
    memset(&ImageInfo, 0, sizeof(ImageInfo));
    ImageInfo.FlashUsed = -1;
    ImageInfo.MeteringMode = -1;
    ImageInfo.Whitebalance = -1;

    // Store file date/time.
    {
        struct stat st;
        if (stat(FileName, &st) >= 0){
            ImageInfo.FileDateTime = st.st_mtime;
            ImageInfo.FileSize = st.st_size;
        }else{
            return;
        }
    }

    if (DoModify || RenameToDate || Exif2FileTime){
        if (access(FileName, 2 /*W_OK*/)){
            gxlogd("Skipping readonly file '%s'\n",FileName);
            return;
        }
    }

    strncpy(ImageInfo.FileName, FileName, JPEG_PATH_MAX);


    if (ApplyCommand || AutoRotate){
        // Applying a command is special - the headers from the file have to be
        // pre-read, then the command executed, and then the image part of the file read.

        if (!ReadJpegFile(FileName, READ_METADATA)) return;

        #ifdef MATTHIAS
            if (AutoResize){
                // Automatic resize computation - to customize for each run...
                if (AutoResizeCmdStuff() == 0){
                    DiscardData();
                    return;
                }
            }
        #endif // MATTHIAS


        if (CheckFileSkip()){
            DiscardData();
            return;
        }

        DiscardAllButExif();

        if (AutoRotate){
            if (DoAutoRotate(FileName)){
                Modified = TRUE;
            }
        }else{
            struct stat dummy;
            //DoCommand(FileName, Quiet ? FALSE : TRUE);

            if (stat(FileName, &dummy)){
                // The file is not there anymore. Perhaps the command
                // was a delete or a move.  So we are all done.
                return;
            }
            Modified = TRUE;
        }
        ReadMode = READ_IMAGE;   // Don't re-read exif section again on next read.

    }else if (ExifXferScrFile){
        char RelativeExifName[JPEG_PATH_MAX+1];

        // Make a relative name.
        RelativeName(RelativeExifName, ExifXferScrFile, FileName);

        if(!ReadJpegFile(RelativeExifName, READ_METADATA)) return;

        DiscardAllButExif();    // Don't re-read exif section again on next read.

        Modified = TRUE;
        ReadMode = READ_IMAGE;
    }

    if (DoModify){
        ReadMode |= READ_IMAGE;
    }

    if (!ReadJpegFile(FileName, ReadMode)) return;

    if (CheckFileSkip()){
        DiscardData();
        return;
    }

    FileSequence += 1; // Count files processed.

    if (ShowConcise){
        ShowConciseImageInfo();
    }else{
        if (!(DoModify || DoReadAction) || ShowTags){
            //ShowImageInfo(ShowFileInfo);

            {
                // if IPTC section is present, show it also.
                Section_t * IptcSection;
                IptcSection = FindSection(M_IPTC);

                if (IptcSection){
                    show_IPTC(IptcSection->Data, IptcSection->Size);
                }
            }
            gxlogd("\n");
        }
    }

    if (ThumbSaveName){
        char OutFileName[JPEG_PATH_MAX+1];
        // Make a relative name.
        RelativeName(OutFileName, ThumbSaveName, FileName);

        if (SaveThumbnail(OutFileName)){
            gxlogd("Created: '%s'\n", OutFileName);
        }
    }

    if (CreateExifSection){
        // Make a new minimal exif section
        create_EXIF();
        Modified = TRUE;
    }

    if (RegenThumbnail){
        if (RegenerateThumbnail(FileName)){
            Modified = TRUE;
        }
    }

    if (ThumbInsertName){
        char ThumbFileName[JPEG_PATH_MAX+1];
        // Make a relative name.
        RelativeName(ThumbFileName, ThumbInsertName, FileName);

        if (ReplaceThumbnail(ThumbFileName)){
            Modified = TRUE;
        }
    }else if (TrimExif){
        // Deleting thumbnail is just replacing it with a null thumbnail.
        if (ReplaceThumbnail(NULL)){
            Modified = TRUE;
        }
    }

    if (
#ifdef MATTHIAS
        AddComment || RemComment ||
#endif
                   EditComment || CommentInsertfileName || CommentInsertLiteral){

        Section_t * CommentSec;
        char Comment[MAX_COMMENT_SIZE+1] = {0};
        int CommentSize = 0;

        CommentSec = FindSection(M_COM);

        if (CommentSec == NULL){
            unsigned char * DummyData;

            DummyData = (uchar *) GUI_MALLOC(3);
            DummyData[0] = 0;
            DummyData[1] = 2;
            DummyData[2] = 0;
            CommentSec = CreateSection(M_COM, DummyData, 2);
        }

        CommentSize = CommentSec->Size-2;
        if (CommentSize > MAX_COMMENT_SIZE){
            gxlogi ( "Truncating comment at %d chars\n",MAX_COMMENT_SIZE);
            CommentSize = MAX_COMMENT_SIZE;
        }

        if (CommentInsertfileName){
            // Read a new comment section from file.
            char CommentFileName[JPEG_PATH_MAX+1];
            FILE * CommentFile;

            // Make a relative name.
            RelativeName(CommentFileName, CommentInsertfileName, FileName);

            CommentFile = fopen(CommentFileName,"r");
            if (CommentFile == NULL){
                gxlogd("Could not open '%s'\n",CommentFileName);
            }else{
                // Read it in.
                // Replace the section.
                CommentSize = fread(Comment, 1, 999, CommentFile);
                fclose(CommentFile);
                if (CommentSize < 0) CommentSize = 0;
            }
        }else if (CommentInsertLiteral){
            strncpy(Comment, CommentInsertLiteral, MAX_COMMENT_SIZE);
            CommentSize = strlen(Comment);
        }else{
#ifdef MATTHIAS
            char CommentZt[MAX_COMMENT_SIZE+1];
            memcpy(CommentZt, (char *)CommentSec->Data+2, CommentSize);
            CommentZt[CommentSize] = '\0';
            if (ModifyDescriptComment(Comment, CommentZt)){
                Modified = TRUE;
                CommentSize = strlen(Comment);
            }
            if (EditComment)
#else
            memcpy(Comment, (char *)CommentSec->Data+2, CommentSize);
#endif
            {
                char EditFileName[JPEG_PATH_MAX+5];
                strcpy(EditFileName, FileName);
                strcat(EditFileName, ".txt");

                CommentSize = FileEditComment(EditFileName, Comment, CommentSize);
            }
        }

        if (strcmp(Comment, (char *)CommentSec->Data+2)){
            // Discard old comment section and put a new one in.
            int size;
            size = CommentSize+2;
            GUI_FREE(CommentSec->Data);
            CommentSec->Data = NULL;
            CommentSec->Size = size;
            CommentSec->Data = GUI_MALLOC(size);
            CommentSec->Data[0] = (uchar)(size >> 8);
            CommentSec->Data[1] = (uchar)(size);
            memcpy((CommentSec->Data)+2, Comment, size-2);
            Modified = TRUE;
        }
        if (!Modified){
            gxlogd("Comment not modified\n");
        }
    }


    if (CommentSavefileName){
        Section_t * CommentSec;
        CommentSec = FindSection(M_COM);

        if (CommentSec != NULL){
            char OutFileName[JPEG_PATH_MAX+1];
            FILE * CommentFile;

            // Make a relative name.
            RelativeName(OutFileName, CommentSavefileName, FileName);

            CommentFile = fopen(OutFileName,"w");

            if (CommentFile){
                fwrite((char *)CommentSec->Data+2, CommentSec->Size-2 ,1, CommentFile);
                fclose(CommentFile);
            }else{
                ErrFatal("Could not write comment file");
            }
        }else{
            gxlogd("File '%s' contains no comment section\n",FileName);
        }
    }

    if (ExifTimeAdjust || ExifTimeSet || DateSetChars || FileTimeToExif){
       if (ImageInfo.numDateTimeTags){
            struct tm tm;
            time_t UnixTime;
            char TempBuf[50];
            int a;
            Section_t * ExifSection;
            if (ExifTimeSet){
                // A time to set was specified.
                UnixTime = ExifTimeSet;
            }else{
                if (FileTimeToExif){
                    FileTimeAsString(ImageInfo.DateTime);
                }
                if (DateSetChars){
                    memcpy(ImageInfo.DateTime, DateSet, DateSetChars);
                    a = 1970;
                    sscanf(DateSet, "%d", &a);
                    if (a < 1970){
                        strcpy(TempBuf, ImageInfo.DateTime);
                        goto skip_unixtime;
                    }
                }
                // A time offset to adjust by was specified.
                if (!Exif2tm(&tm, ImageInfo.DateTime)) goto badtime;

                // Convert to unix 32 bit time value, add offset, and convert back.
                UnixTime = mktime(&tm);
                if ((int)UnixTime == -1) goto badtime;
                UnixTime += ExifTimeAdjust;
            }
            tm = *localtime(&UnixTime);

            // Print to temp buffer first to avoid putting null termination in destination.
            // snprintf() would do the trick, hbut not available everywhere (like FreeBSD 4.4)
            sprintf(TempBuf, "%04d:%02d:%02d %02d:%02d:%02d",
                tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec);

skip_unixtime:
            ExifSection = FindSection(M_EXIF);

            for (a = 0; a < ImageInfo.numDateTimeTags; a++) {
                uchar * Pointer;
                Pointer = ExifSection->Data+ImageInfo.DateTimeOffsets[a]+8;
                memcpy(Pointer, TempBuf, 19);
            }
            memcpy(ImageInfo.DateTime, TempBuf, 19);

            Modified = TRUE;
        }else{
            gxlogd("File '%s' contains no Exif timestamp to change\n", FileName);
        }
    }

    if (DeleteComments){
        if (RemoveSectionType(M_COM)) Modified = TRUE;
    }
    if (DeleteExif){
        if (RemoveSectionType(M_EXIF)) Modified = TRUE;
    }
    if (DeleteIptc){
        if (RemoveSectionType(M_IPTC)) Modified = TRUE;
    }
    if (DeleteXmp){
        if (RemoveSectionType(M_XMP)) Modified = TRUE;
    }
    if (DeleteUnknown){
        if (RemoveUnknownSections()) Modified = TRUE;
    }


    if (Modified){
        char BackupName[JPEG_PATH_MAX+5];
        struct stat buf;

        if (!Quiet) gxlogd("Modified: %s\n",FileName);

        strcpy(BackupName, FileName);
        strcat(BackupName, ".t");

        // Remove any .old file name that may pre-exist
        unlink(BackupName);

        // Rename the old file.
        rename(FileName, BackupName);

        // Write the new file.
        WriteJpegFile(FileName);

        // Copy the access rights from original file
        if (stat(BackupName, &buf) == 0){
            // set Unix access rights and time to new file
            //struct utimbuf mtime;
            chmod(FileName, buf.st_mode);

            //mtime.actime = buf.st_mtime;
            //mtime.modtime = buf.st_mtime;

            //utime(FileName, &mtime);
        }

        // Now that we are done, remove original file.
        unlink(BackupName);
    }


    if (Exif2FileTime){
        // Set the file date to the date from the exif header.
        if (ImageInfo.numDateTimeTags){
            // Converte the file date to Unix time.
            struct tm tm;
            time_t UnixTime;
            //struct utimbuf mtime;
            if (!Exif2tm(&tm, ImageInfo.DateTime)) goto badtime;

            UnixTime = mktime(&tm);
            if ((int)UnixTime == -1){
                goto badtime;
            }

            //mtime.actime = UnixTime;
            //mtime.modtime = UnixTime;

            /*if (utime(FileName, &mtime) != 0){
                gxlogd("Error: Could not change time of file '%s'\n",FileName);
            }else{
                if (!Quiet) gxlogd("%s\n",FileName);
            }*/
        }else{
            gxlogd("File '%s' contains no Exif timestamp\n", FileName);
        }
    }

    // Feature to rename image according to date and time from camera.
    // I use this feature to put images from multiple digicams in sequence.

    if (RenameToDate){
        DoFileRenaming(FileName);
    }
    DiscardData();
    return;
badtime:
    gxlogd("Error: Time '%s': cannot convert to Unix time\n",ImageInfo.DateTime);
    DiscardData();
#endif/*GUI_JPEG_INFO*/
}

//--------------------------------------------------------------------------
// Parse specified date or date+time from command line.
//--------------------------------------------------------------------------
time_t ParseCmdDate(char * DateSpecified)
{
    int a;
    struct tm tm;
    time_t UnixTime;

    tm.tm_wday = -1;
    tm.tm_hour = tm.tm_min = tm.tm_sec = 0;

    a = sscanf(DateSpecified, "%d:%d:%d/%d:%d:%d",
            &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
            &tm.tm_hour, &tm.tm_min, &tm.tm_sec);

    if (a != 3 && a < 5){
        // Date must be YYYY:MM:DD, YYYY:MM:DD+HH:MM
        // or YYYY:MM:DD+HH:MM:SS
        ErrFatal("Could not parse specified date");
    }
    tm.tm_isdst = -1;
    tm.tm_mon -= 1;      // Adjust for unix zero-based months
    tm.tm_year -= 1900;  // Adjust for year starting at 1900

    UnixTime = mktime(&tm);
    if (UnixTime == -1){
        ErrFatal("Specified time is invalid or out of range");
    }

    return UnixTime;
}


