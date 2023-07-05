/*
 *    This file is part of MotionPlus.
 *
 *    MotionPlus is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    MotionPlus is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with MotionPlus.  If not, see <https://www.gnu.org/licenses/>.
 *
 *    Copyright 2020-2023 MotionMrDave@gmail.com
 *
*/

#include "motionplus.hpp"
#include "conf.hpp"
#include "util.hpp"
#include "logger.hpp"
#include "alg_sec.hpp" /* For sec detect in format output */


/** Non case sensitive equality check for strings*/
int mystrceq(const char* var1, const char* var2)
{
    if ((var1 == NULL) || (var2 == NULL)) {
        return 0;
    }
    return (strcasecmp(var1,var2) ? 0 : 1);
}

/** Non case sensitive inequality check for strings*/
int mystrcne(const char* var1, const char* var2)
{
    if ((var1 == NULL) || (var2 == NULL)) {
        return 0;
    }
    return (strcasecmp(var1,var2) ? 1: 0);
}

/** Case sensitive equality check for strings*/
int mystreq(const char* var1, const char* var2)
{
    if ((var1 == NULL) || (var2 == NULL)) {
        return 0;
    }
    return (strcmp(var1,var2) ? 0 : 1);
}

/** Case sensitive inequality check for strings*/
int mystrne(const char* var1, const char* var2)
{
    if ((var1 == NULL) ||(var2 == NULL)) {
        return 0;
    }
    return (strcmp(var1,var2) ? 1: 0);
}

/* Trim whitespace from left side */
void myltrim(std::string &parm)
{
    if (parm.length() == 0 ) {
        return;
    }

    while (parm.substr(0, 1) == " ") {
        if (parm.length() == 1) {
            parm="";
            return;
        } else {
            parm = parm.substr(1);
        }
    }
}

/* Trim whitespace from right side */
void myrtrim(std::string &parm)
{
    if (parm.length() == 0 ) {
        return;
    }

    while (parm.substr(parm.length()-1,1) == " ") {
        if (parm.length() == 1) {
            parm="";
            return;
        } else {
            parm = parm.substr(0,parm.length()-1);
        }
    }
}

/* Trim left and right whitespace */
void mytrim(std::string &parm)
{
    myrtrim(parm);
    myltrim(parm);
}

/* Remove surrounding quotes */
void myunquote(std::string &parm)
{
    size_t plen;

    mytrim(parm);

    plen = parm.length();
    while ((plen >= 2) &&
        (((parm.substr(0,1)== "\"") && (parm.substr(plen,1)== "\"")) ||
         ((parm.substr(0,1)== "'") && (parm.substr(plen,1)== "'")))) {

        parm = parm.substr(1, plen-2);
        plen = parm.length();
    }

}


/* Free memory and set the pointer to NULL*/
void myfree(void *ptr_addr) {
    void **ptr = (void **)ptr_addr;

    if (*ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
}

/** mymalloc */
void *mymalloc(size_t nbytes)
{
    void *dummy = calloc(nbytes, 1);

    if (!dummy) {
        MOTPLS_LOG(EMG, TYPE_ALL, SHOW_ERRNO, _("Could not allocate %llu bytes of memory!")
            ,(unsigned long long)nbytes);
        exit(1);
    }

    return dummy;
}

/** myrealloc */
void *myrealloc(void *ptr, size_t size, const char *desc)
{
    void *dummy = NULL;

    if (size == 0) {
        free(ptr);
        MOTPLS_LOG(WRN, TYPE_ALL, NO_ERRNO
            ,_("Warning! Function %s tries to resize memoryblock at %p to 0 bytes!")
            ,desc, ptr);
    } else {
        dummy = realloc(ptr, size);
        if (!dummy) {
            MOTPLS_LOG(EMG, TYPE_ALL, NO_ERRNO
                ,_("Could not resize memory-block at offset %p to %llu bytes (function %s)!")
                ,ptr, (unsigned long long)size, desc);
            exit(1);
        }
    }

    return dummy;
}


/**
 * mycreate_path
 *   Create whole path.
 *   Path provided *must* end with a slash!
 */
int mycreate_path(const char *path)
{
    std::string tmp;
    mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    size_t indx_pos;
    int retcd;
    struct stat statbuf;

    tmp = std::string(path);

    if (tmp.substr(0, 1) == "/") {
        indx_pos = tmp.find("/", 1);
    } else {
        indx_pos = tmp.find("/", 0);
    }

    while (indx_pos != std::string::npos) {
        if (stat(tmp.substr(0, indx_pos + 1).c_str(), &statbuf) != 0) {
            MOTPLS_LOG(NTC, TYPE_ALL, NO_ERRNO
                ,_("Creating %s"), tmp.substr(0, indx_pos + 1).c_str());
            retcd = mkdir(tmp.substr(0, indx_pos + 1).c_str(), mode);
            if (retcd == -1 && errno != EEXIST) {
                MOTPLS_LOG(ERR, TYPE_ALL, SHOW_ERRNO
                    ,_("Problem creating directory %s")
                    , tmp.substr(0, indx_pos + 1).c_str());
                return -1;
            }
        }
        indx_pos++;
        if (indx_pos >= tmp.length()) {
            break;
        }
        indx_pos = tmp.find("/", indx_pos);
    }

    return 0;
}

/* myfopen */
FILE *myfopen(const char *path, const char *mode)
{
    FILE *fp;

    fp = fopen(path, mode);
    if (fp) {
        return fp;
    }

    /* If path did not exist, create and try again*/
    if (errno == ENOENT) {
        if (mycreate_path(path) == -1) {
            return NULL;
        }
        fp = fopen(path, mode);
    }
    if (!fp) {
        MOTPLS_LOG(ERR, TYPE_ALL, SHOW_ERRNO
            ,_("Error opening file %s with mode %s"), path, mode);
        return NULL;
    }

    return fp;
}

/** myfclose */
int myfclose(FILE* fh)
{
    int rval = fclose(fh);

    if (rval != 0) {
        MOTPLS_LOG(ERR, TYPE_ALL, SHOW_ERRNO, _("Error closing file"));
    }

    return rval;
}

/**
 * mystrftime_long
 *
 *   Motion-specific long form of format specifiers.
 *
 * Parameters:
 *
 *   cam        - current thread's context structure.
 *   width      - width associated with the format specifier.
 *   word       - beginning of the format specifier's word.
 *   l          - length of the format specifier's word.
 *   out        - output buffer where to store the result. Size: PATH_MAX.
 *
 * This is called if a format specifier with the format below was found:
 *
 *   % { word }
 *
 * As a special edge case, an incomplete format at the end of the string
 * is processed as well:
 *
 *   % { word \0
 *
 * Any valid format specified width is supported, e.g. "%12{host}".
 *
 * The following specifier keywords are currently supported:
 *
 * host    Replaced with the name of the local machine (see gethostname(2)).
 * fps     Equivalent to %fps.
 */
static void mystrftime_long (const ctx_dev *cam,
        int width, const char *word, int l, char *out)
{

    #define SPECIFIERWORD(k) ((strlen(k)==l) && (!strncmp (k, word, l)))

    if (SPECIFIERWORD("host")) {
        snprintf (out, PATH_MAX, "%*s", width, cam->hostname);
        return;
    }
    if (SPECIFIERWORD("fps")) {
        sprintf(out, "%*d", width, cam->movie_fps);
        return;
    }
    if (SPECIFIERWORD("eventid")) {
        sprintf(out, "%*s", width, cam->eventid);
        return;
    }
    if (SPECIFIERWORD("ver")) {
        sprintf(out, "%*s", width, VERSION);
        return;
    }
    if (SPECIFIERWORD("sdevx")) {
        sprintf(out, "%*d", width,  cam->current_image->location.stddev_x);
        return;
    }
    if (SPECIFIERWORD("sdevy")) {
        sprintf(out, "%*d", width,  cam->current_image->location.stddev_y);
        return;
    }
    if (SPECIFIERWORD("sdevxy")) {
        sprintf(out, "%*d", width,  cam->current_image->location.stddev_xy);
        return;
    }
    if (SPECIFIERWORD("ratio")) {
        sprintf(out, "%*d", width,  cam->current_image->diffs_ratio);
        return;
    }
    if (SPECIFIERWORD("action_user")) {
        sprintf(out, "%*s", width,  cam->action_user);
        return;
    }

    if (SPECIFIERWORD("secdetect")) {
        if (cam->algsec_inuse) {
            if (cam->algsec->isdetected) {
                sprintf(out, "%*s", width, "Y");
            } else {
                sprintf(out, "%*s", width, "N");
            }
        } else {
            sprintf(out, "%*s", width, "N");
        }
        return;
    }

    if (SPECIFIERWORD("trig_freq")) {
        if (cam->snd_info != NULL) {
            snprintf (out, PATH_MAX, "%*s", width, cam->snd_info->trig_freq.c_str() );
        }
        return;
    }

    if (SPECIFIERWORD("trig_nbr")) {
        if (cam->snd_info != NULL) {
            snprintf (out, PATH_MAX, "%*s", width, cam->snd_info->trig_nbr.c_str() );
        }
        return;
    }

    if (SPECIFIERWORD("trig_nm")) {
        if (cam->snd_info != NULL) {
            snprintf (out, PATH_MAX, "%*s", width, cam->snd_info->trig_nm.c_str() );
        }
        return;
    }


    // Not a valid modifier keyword. Log the error and ignore.
    MOTPLS_LOG(ERR, TYPE_ALL, NO_ERRNO,
        _("invalid format specifier keyword %*.*s"), l, l, word);

    // Do not let the output buffer empty, or else where to restart the
    // interpretation of the user string will become dependent to far too
    // many conditions. Maybe change loop to "if (*pos_userformat == '%') {
    // ...} __else__ ..."?
    out[0] = '~'; out[1] = 0;
}

/**
 * mystrftime
 *
 *   Motion-specific variant of strftime(3) that supports additional format
 *   specifiers in the format string.
 *
 * Parameters:
 *
 *   cam        - current thread's context structure
 *   s          - destination string
 *   max        - max number of bytes to write
 *   userformat - format string
 *   tm         - time information
 *   filename   - string containing full path of filename
 *                set this to NULL if not relevant
 *   sqltype    - Filetype as used in SQL feature, set to 0 if not relevant
 *
 * Returns: number of bytes written to the string s
 */
size_t mystrftime(ctx_dev *cam, char *s, size_t max, const char *userformat,
        const struct timespec *ts1, const char *filename, int sqltype)
{
    char formatstring[PATH_MAX] = "";
    char tempstring[PATH_MAX] = "";
    char *format, *tempstr;
    const char *pos_userformat;
    int width;
    struct tm timestamp_tm;

    localtime_r(&ts1->tv_sec, &timestamp_tm);

    format = formatstring;

    /* if mystrftime is called with userformat = NULL we return a zero length string */
    if (userformat == NULL) {
        *s = '\0';
        return 0;
    }

    for (pos_userformat = userformat; *pos_userformat; ++pos_userformat) {

        if (*pos_userformat == '%') {
            /*
             * Reset 'tempstr' to point to the beginning of 'tempstring',
             * otherwise we will eat up tempstring if there are many
             * format specifiers.
             */
            tempstr = tempstring;
            tempstr[0] = '\0';
            width = 0;
            while ('0' <= pos_userformat[1] && pos_userformat[1] <= '9') {
                width *= 10;
                width += pos_userformat[1] - '0';
                ++pos_userformat;
            }

            switch (*++pos_userformat) {
            case '\0': // end of string
                --pos_userformat;
                break;

            case 'v': // event
                sprintf(tempstr, "%0*d", width ? width : 2, cam->event_nr);
                break;

            case 'q': // shots
                sprintf(tempstr, "%0*d", width ? width : 2,
                    cam->current_image->shot);
                break;

            case 'D': // diffs
                sprintf(tempstr, "%*d", width, cam->current_image->diffs);
                break;

            case 'N': // noise
                sprintf(tempstr, "%*d", width, cam->noise);
                break;

            case 'i': // motion width
                sprintf(tempstr, "%*d", width,
                    cam->current_image->location.width);
                break;

            case 'J': // motion height
                sprintf(tempstr, "%*d", width,
                    cam->current_image->location.height);
                break;

            case 'K': // motion center x
                sprintf(tempstr, "%*d", width, cam->current_image->location.x);
                break;

            case 'L': // motion center y
                sprintf(tempstr, "%*d", width, cam->current_image->location.y);
                break;

            case 'o': // threshold
                sprintf(tempstr, "%*d", width, cam->threshold);
                break;

            case 'Q': // number of labels
                sprintf(tempstr, "%*d", width,
                    cam->current_image->total_labels);
                break;

            case 't': // device id
                sprintf(tempstr, "%*d", width, cam->device_id);
                break;

            case 'C': // text_event
                if (cam->text_event_string[0]) {
                    snprintf(tempstr, PATH_MAX, "%*s", width,
                        cam->text_event_string);
                } else {
                    ++pos_userformat;
                }
                break;

            case 'w': // picture width
                sprintf(tempstr, "%*d", width, cam->imgs.width);
                break;

            case 'h': // picture height
                sprintf(tempstr, "%*d", width, cam->imgs.height);
                break;

            case 'f': // filename
                if (filename) {
                    snprintf(tempstr, PATH_MAX, "%*s", width, filename);
                } else {
                    ++pos_userformat;
                }
                break;

            case 'n': // sqltype
                if (sqltype) {
                    sprintf(tempstr, "%*d", width, sqltype);
                } else {
                    ++pos_userformat;
                }
                break;

            case '{': // long format specifier word.
                {
                    const char *word = ++pos_userformat;
                    while ((*pos_userformat != '}') && (*pos_userformat != 0))
                        ++pos_userformat;
                    mystrftime_long (cam, width, word, (int)(pos_userformat-word), tempstr);
                    if (*pos_userformat == '\0') {
                        --pos_userformat;
                    }
                }
                break;

            case '$': // thread name
                if (cam->conf->device_name != "") {
                    cam->conf->device_name.copy(tempstr, PATH_MAX);
                } else {
                    ++pos_userformat;
                }
                break;

            default: // Any other code is copied with the %-sign
                *format++ = '%';
                *format++ = *pos_userformat;
                continue;
            }

            /*
             * If a format specifier was found and used, copy the result from
             * 'tempstr' to 'format'.
             */
            if (tempstr[0]) {
                while ((*format = *tempstr++) != '\0') {
                    ++format;
                }
                continue;
            }
        }

        /* For any other character than % we just simply copy the character */
        *format++ = *pos_userformat;
    }

    *format = '\0';
    format = formatstring;

    return strftime(s, max, format, &timestamp_tm);
}

void mythreadname_set(const char *abbr, int threadnbr, const char *threadname)
{
    /* When the abbreviation is sent in as null, that means we are being
     * provided a fully filled out thread name (usually obtained from a
     * previously called get_threadname so we set it without additional
     *  formatting.
     */

    char tname[32];
    if (abbr != NULL) {
        snprintf(tname, sizeof(tname), "%s%02d%s%s",abbr,threadnbr,
             threadname ? ":" : "",
             threadname ? threadname : "");
    } else {
        snprintf(tname, sizeof(tname), "%s",threadname);
    }

    #ifdef __APPLE__
        pthread_setname_np(tname);
    #elif defined(BSD)
        pthread_set_name_np(pthread_self(), tname);
    #elif HAVE_PTHREAD_SETNAME_NP
        pthread_setname_np(pthread_self(), tname);
    #else
        MOTPLS_LOG(INF, TYPE_NETCAM, NO_ERRNO, _("Unable to set thread name %s"), tname);
    #endif

}

void mythreadname_get(char *threadname)
{
    #if ((!defined(BSD) && HAVE_PTHREAD_GETNAME_NP) || defined(__APPLE__))
        char currname[16];
        pthread_getname_np(pthread_self(), currname, sizeof(currname));
        snprintf(threadname, sizeof(currname), "%s",currname);
    #else
        snprintf(threadname, 8, "%s","Unknown");
    #endif
}

bool mycheck_passthrough(ctx_dev *cam)
{
    #if (MYFFVER >= 57041)
        if (cam->movie_passthrough) {
            return true;
        } else {
            return false;
        }
    #else
        if (cam->movie_passthrough) {
            MOTPLS_LOG(INF, TYPE_NETCAM, NO_ERRNO
                ,_("FFMPEG version too old. Disabling pass-through processing."));
        }
        return false;
    #endif

}

static void mytranslate_locale_chg(const char *langcd)
{
    #ifdef HAVE_GETTEXT
        /* This routine is for development testing only.  It is not used for
        * regular users because once this locale is change, it changes the
        * whole computer over to the new locale.  Therefore, we just return
        */
        return;

        setenv ("LANGUAGE", langcd, 1);
        /* Invoke external function to change locale*/
        ++_nl_msg_cat_cntr;
    #else
        if (langcd != NULL) {
            MOTPLS_LOG(NTC, TYPE_ALL, NO_ERRNO,"No native language support");
        }
    #endif
}

void mytranslate_init(void)
{
    #ifdef HAVE_GETTEXT
        mytranslate_text("", 1);
        setlocale (LC_ALL, "");

        //translate_locale_chg("li");
        mytranslate_locale_chg("es");

        bindtextdomain ("motion", LOCALEDIR);
        bind_textdomain_codeset ("motion", "UTF-8");
        textdomain ("motion");

        MOTPLS_LOG(NTC, TYPE_ALL, NO_ERRNO,_("Language: English"));

    #else
        /* Disable native language support */
        mytranslate_text("", 0);

        /* This avoids a unused function warning */
        mytranslate_locale_chg("en");
    #endif
}

char* mytranslate_text(const char *msgid, int setnls)
{
    static bool nls_enabled = true;

    if (setnls == 0) {
        if (nls_enabled) {
            MOTPLS_LOG(NTC, TYPE_ALL, NO_ERRNO,_("Disabling native language support"));
        }
        nls_enabled = false;
        return NULL;

    } else if (setnls == 1) {
        if (!nls_enabled) {
            MOTPLS_LOG(NTC, TYPE_ALL, NO_ERRNO,_("Enabling native language support"));
        }
        nls_enabled = true;
        return NULL;

    } else {
        #ifdef HAVE_GETTEXT
            if (nls_enabled) {
                return (char*)gettext(msgid);
            } else {
                return (char*)msgid;
            }
        #else
            return (char*)msgid;
        #endif
    }
}

/****************************************************************************
 *  The section below is the "my" section of functions.
 *  These are designed to be extremely simple version specific
 *  variants of the libav functions.
 ****************************************************************************/

/*********************************************/
AVFrame *myframe_alloc(void)
{
    AVFrame *pic;
    #if (MYFFVER >= 55000)
        pic = av_frame_alloc();
    #else
        pic = avcodec_alloc_frame();
    #endif
    return pic;
}
/*********************************************/
void myframe_free(AVFrame *frame)
{
    #if (MYFFVER >= 55000)
        av_frame_free(&frame);
    #else
        av_freep(&frame);
    #endif
}
/*********************************************/
int myimage_get_buffer_size(enum MyPixelFormat pix_fmt, int width, int height)
{
    int retcd = 0;
    #if (MYFFVER >= 57000)
        int align = 1;
        retcd = av_image_get_buffer_size(pix_fmt, width, height, align);
    #else
        retcd = avpicture_get_size(pix_fmt, width, height);
    #endif
    return retcd;
}
/*********************************************/
int myimage_copy_to_buffer(AVFrame *frame, uint8_t *buffer_ptr, enum MyPixelFormat pix_fmt
        , int width, int height, int dest_size)
{
    int retcd = 0;
    #if (MYFFVER >= 57000)
        int align = 1;
        retcd = av_image_copy_to_buffer((uint8_t *)buffer_ptr,dest_size
            ,(const uint8_t * const*)frame,frame->linesize,pix_fmt,width,height,align);
    #else
        retcd = avpicture_layout((const AVPicture*)frame,pix_fmt,width,height
            ,(unsigned char *)buffer_ptr,dest_size);
    #endif
    return retcd;
}
/*********************************************/
int myimage_fill_arrays(AVFrame *frame,uint8_t *buffer_ptr,enum MyPixelFormat pix_fmt
        , int width,int height)
{
    int retcd = 0;
    #if (MYFFVER >= 57000)
        int align = 1;
        retcd = av_image_fill_arrays(
            frame->data
            ,frame->linesize
            ,buffer_ptr
            ,pix_fmt
            ,width
            ,height
            ,align
        );
    #else
        retcd = avpicture_fill(
            (AVPicture *)frame
            ,buffer_ptr
            ,pix_fmt
            ,width
            ,height);
    #endif
    return retcd;
}
/*********************************************/
void mypacket_free(AVPacket *pkt)
{
    #if (MYFFVER >= 57041)
        av_packet_free(&pkt);
    #else
        av_free_packet(pkt);
    #endif

}
/*********************************************/
void myavcodec_close(AVCodecContext *codec_context)
{
    #if (MYFFVER >= 57041)
        avcodec_free_context(&codec_context);
    #else
        avcodec_close(codec_context);
    #endif
}
/*********************************************/
int mycopy_packet(AVPacket *dest_pkt, AVPacket *src_pkt)
{
    #if (MYFFVER >= 55000)
        return av_packet_ref(dest_pkt, src_pkt);
    #else
        /* Old versions of libav do not support copying packet
        * We therefore disable the pass through recording and
        * for this function, simply do not do anything
        */
        if (dest_pkt == src_pkt ) {
            return 0;
        } else {
            return 0;
        }
    #endif
}
/*********************************************/
AVPacket *mypacket_alloc(AVPacket *pkt)
{
    if (pkt != NULL) {
        mypacket_free(pkt);
    };
    pkt = av_packet_alloc();
    #if (MYFFVER < 58076)
        av_init_packet(pkt);
        pkt->data = NULL;
        pkt->size = 0;
    #endif

    return pkt;

}

/*********************************************/

void util_parms_free(ctx_params *params)
{
    int indx_parm;

    if (params == NULL ) {
        return;
    }

    for (indx_parm=0; indx_parm<params->params_count; indx_parm++) {
        myfree(&params->params_array[indx_parm].param_name);
        myfree(&params->params_array[indx_parm].param_value);
    }
    myfree(&params->params_array);

    params->params_count = 0;

}
static void util_parms_file(ctx_params *params, const char *params_file)
{
    int indx, chk;
    size_t stpos;
    std::string line, parm_nm, parm_vl;
    std::ifstream ifs;

    MOTPLS_LOG(ERR, TYPE_ALL, NO_ERRNO
                , _("parse file: %s"), params_file);
    chk = 0;
    for (indx = 0; indx < params->params_count; indx++) {
        if (mystreq(params->params_array[indx].param_name, "params_file") ) {
            chk++;
        }
    }
    if (chk > 1){
        MOTPLS_LOG(ERR, TYPE_ALL, NO_ERRNO
            ,_("Only one params_file specification is permitted."));
        return;
    }

    ifs.open(params_file);
        if (ifs.is_open() == false) {
            MOTPLS_LOG(ERR, TYPE_ALL, NO_ERRNO
                , _("params_file not found: %s"), params_file);
            return;
        }
        while (std::getline(ifs, line)) {
            mytrim(line);
            stpos = line.find(" ");
            if (stpos > line.find("=")) {
                stpos = line.find("=");
            }
            if ((stpos != std::string::npos) &&
                (stpos != line.length()-1) &&
                (stpos != 0) &&
                (line.substr(0, 1) != ";") &&
                (line.substr(0, 1) != "#")) {
                parm_nm = line.substr(0, stpos);
                parm_vl = line.substr(stpos+1, line.length()-stpos);
                mytrim(parm_nm);
                mytrim(parm_vl);
                util_parms_add(params, parm_nm.c_str(), parm_vl.c_str());
            } else if ((line != "") &&
                (line.substr(0, 1) != ";") &&
                (line.substr(0, 1) != "#")) {
                MOTPLS_LOG(ERR, TYPE_ALL, NO_ERRNO
                , _("Unable to parse line: %s"), line.c_str());
            }
        }
    ifs.close();

}

void util_parms_add(ctx_params *params, const char *parm_nm, const char *parm_val)
{
    int indx;

    for (indx = 0; indx < params->params_count; indx++) {
        if (mystreq(params->params_array[indx].param_name, parm_nm) ) {
            break;
        }
    }

    if (indx == params->params_count) {
        params->params_count++;
        if (params->params_count == 1) {
            params->params_array =(ctx_params_item *) mymalloc(sizeof(ctx_params_item));
        } else {
            params->params_array =(ctx_params_item *)realloc(params->params_array
                , sizeof(ctx_params_item)*params->params_count);
        }
    } else {
        free(params->params_array[indx].param_name);
        free(params->params_array[indx].param_value);
    }

    if (parm_nm != NULL) {
        params->params_array[indx].param_name =(char*)mymalloc(strlen(parm_nm)+1);
        sprintf(params->params_array[indx].param_name,"%s",parm_nm);
    } else {
        params->params_array[indx].param_name = NULL;
    }

    if (parm_val != NULL) {
        params->params_array[indx].param_value =(char*)mymalloc(strlen(parm_val)+1);
        sprintf(params->params_array[indx].param_value,"%s",parm_val);
    } else {
        params->params_array[indx].param_value = NULL;
    }

    MOTPLS_LOG(INF, TYPE_ALL, NO_ERRNO,_("Parsed: >%s< >%s<")
        ,params->params_array[params->params_count-1].param_name
        ,params->params_array[params->params_count-1].param_value);

    if (mystrceq(parm_nm, "params_file") && (parm_val != NULL)) {
        util_parms_file(params, parm_val);
    }
}

static void util_parms_strip_qte(std::string &parm)
{

    if (parm.length() == 0) {
        return;
    }

    if (parm.substr(0, 1)=="\"") {
        if (parm.length() == 1) {
            parm = "";
            return;
        } else {
            parm = parm.substr(1);
        }
    }

    if (parm.substr(parm.length() -1, 1)== "\"") {
        if (parm.length() == 1) {
            parm = "";
            return;
        } else {
            parm = parm.substr(0, parm.length() - 1);
        }
    }

}

static void util_parms_extract(ctx_params *params, std::string &parmline
        ,size_t indxnm_st,size_t indxnm_en,size_t indxvl_st,size_t indxvl_en)
{
    std::string parm_nm, parm_vl;

    if ((indxnm_st != std::string::npos) &&
        (indxnm_en != std::string::npos) &&
        (indxvl_st != std::string::npos) &&
        (indxvl_en != std::string::npos)) {

        parm_nm = parmline.substr(indxnm_st, indxnm_en - indxnm_st + 1);
        parm_vl = parmline.substr(indxvl_st, indxvl_en - indxvl_st + 1);

        mytrim(parm_nm);
        mytrim(parm_vl);

        util_parms_strip_qte(parm_nm);
        util_parms_strip_qte(parm_vl);

        util_parms_add(params, parm_nm.c_str(), parm_vl.c_str());

    }

}

/* Cut out the parameter that was just extracted from the parmline string */
static void util_parms_next(std::string &parmline, size_t indxnm_st, size_t indxvl_en)
{
    size_t indxcm;

    indxcm = parmline.find(",", indxvl_en);

    if (indxnm_st == 0) {
        if (indxcm == std::string::npos) {
            parmline = "";
        } else {
            parmline = parmline.substr(indxcm + 1);
        }
    } else {
        if (indxcm == std::string::npos) {
            parmline = parmline.substr(0, indxnm_st - 1);
        } else {
            parmline = parmline.substr(0, indxnm_st - 1) + parmline.substr(indxcm + 1);
        }
    }
    mytrim(parmline);

}

void util_parms_parse_qte(ctx_params *params, std::string &parmline)
{
    size_t indxnm_st, indxnm_en;
    size_t indxvl_st, indxvl_en;
    size_t indxcm, indxeq;

    /* Parse out all the items within quotes first */
    while (parmline.find("\"", 0) != std::string::npos) {

        indxnm_st = parmline.find("\"", 0);

        indxnm_en = parmline.find("\"", indxnm_st + 1);
        if (indxnm_en == std::string::npos) {
            indxnm_en = parmline.length() - 1;
        }

        indxcm = parmline.find(",", indxnm_en + 1);
        if (indxcm == std::string::npos) {
            indxcm = parmline.length() - 1;
        }

        indxeq = parmline.find("=", indxnm_en + 1);
        if (indxeq == std::string::npos) {
            indxeq = parmline.length() - 1;
        }

        if (indxcm <= indxeq) {
            /* The quoted part of the parm was the value not the name */
            indxvl_st = indxnm_st;
            indxvl_en = indxnm_en;

            indxnm_st = parmline.find_last_of(",", indxvl_st);
            if (indxnm_st == std::string::npos) {
                indxnm_st = 0;
            } else {
                indxnm_st++; /* Move past the comma */
            }

            indxnm_en = parmline.find("=", indxnm_st);
            if ((indxnm_en == std::string::npos) ||
                (indxnm_en > indxvl_st)) {
                indxnm_en = indxvl_st + 1;
            }
            indxnm_en--; /* do not include the = */

        } else {
            /* The quoted part of the parm was the name */
            indxvl_st = parmline.find("\"",indxeq + 1);
            indxcm = parmline.find(",", indxeq + 1);
            if (indxcm == std::string::npos) {
                if (indxvl_st == std::string::npos) {
                    indxvl_st = indxeq + 1;
                    if (indxvl_st >= parmline.length()) {
                        indxvl_st = parmline.length() - 1;
                    }
                    indxvl_en = parmline.length() - 1;
                } else {
                    /* The value is also enclosed in quotes */
                    indxvl_en=parmline.find("\"", indxvl_st + 1);
                    if (indxvl_en == std::string::npos) {
                        indxvl_en = parmline.length() - 1;
                    }
                }
            } else if (indxvl_st == std::string::npos) {
                /* There are no more quotes in the line */
                indxvl_st = indxeq + 1;
                indxvl_en = parmline.find(",",indxvl_st) - 1;
            } else if (indxcm < indxvl_st) {
                /* The quotes belong to next item */
                indxvl_st = indxeq + 1;
                indxvl_en = parmline.find(",",indxvl_st) - 1;
            } else {
                /* The value is also enclosed in quotes */
                indxvl_en=parmline.find("\"", indxvl_st + 1);
                if (indxvl_en == std::string::npos) {
                    indxvl_en = parmline.length() - 1;
                }
            }
        }

        MOTPLS_LOG(DBG, TYPE_ALL, NO_ERRNO,_("Parsing: >%s< >%ld %ld %ld %ld<")
            ,parmline.c_str(), indxnm_st, indxnm_en, indxvl_st, indxvl_en);

        util_parms_extract(params, parmline, indxnm_st, indxnm_en, indxvl_st, indxvl_en);
        util_parms_next(parmline, indxnm_st, indxvl_en);

    }
}

void util_parms_parse_comma(ctx_params *params, std::string &parmline)
{
    size_t indxnm_st, indxnm_en;
    size_t indxvl_st, indxvl_en;

    while (parmline.find(",", 0) != std::string::npos) {
        indxnm_st = 0;
        indxnm_en = parmline.find("=", 1);
        if (indxnm_en == std::string::npos) {
            indxnm_en = 0;
            indxvl_st = 0;
        } else {
            indxvl_st = indxnm_en + 1;  /* Position past = */
            indxnm_en--;                /* Position before = */
        }

        if (parmline.find(",", indxvl_st) == std::string::npos) {
            indxvl_en = parmline.length() - 1;
        } else {
            indxvl_en = parmline.find(",",indxvl_st) - 1;
        }

        MOTPLS_LOG(DBG, TYPE_ALL, NO_ERRNO,_("Parsing: >%s< >%ld %ld %ld %ld<")
            ,parmline.c_str(), indxnm_st, indxnm_en, indxvl_st, indxvl_en);

        util_parms_extract(params, parmline, indxnm_st, indxnm_en, indxvl_st, indxvl_en);
        util_parms_next(parmline, indxnm_st, indxvl_en);
    }

    /* Take care of last parameter */
    if (parmline != "") {
        indxnm_st = 0;
        indxnm_en = parmline.find("=", 1);
        if (indxnm_en == std::string::npos) {
            /* If no value then we are done */
            return;
        } else {
            indxvl_st = indxnm_en + 1;  /* Position past = */
            indxnm_en--;                /* Position before = */
        }
        indxvl_en = parmline.length() - 1;

        MOTPLS_LOG(DBG, TYPE_ALL, NO_ERRNO,_("Parsing: >%s< >%ld %ld %ld %ld<")
            ,parmline.c_str(), indxnm_st, indxnm_en, indxvl_st, indxvl_en);

        util_parms_extract(params, parmline, indxnm_st, indxnm_en, indxvl_st, indxvl_en);
        util_parms_next(parmline, indxnm_st, indxvl_en);
    }

}

/* Parse through the config line and put into the array */
void util_parms_parse(ctx_params *params, std::string confline)
{
    /* Parse through the configuration option to get values
     * The values are separated by commas but may also have
     * double quotes around the names which include a comma.
     * Examples:
     * v4l2_params ID01234= 1, ID23456=2
     * v4l2_params "Brightness, auto" = 1, ID23456=2
     * v4l2_params ID23456=2, "Brightness, auto" = 1,ID2222=5
     */

    std::string parmline;

    if ((params->update_params == false) ||
        (confline == "")) {
        return;
    }
    /* We make a copy because the parsing destroys the value passed */
    parmline = confline;

    MOTPLS_LOG(INF, TYPE_ALL, NO_ERRNO,_("Starting parsing parameters"));

    util_parms_free(params);

    util_parms_parse_qte(params, parmline);

    util_parms_parse_comma(params, parmline);

    params->update_params = false;

    return;

}

void util_parms_add_default(ctx_params *params, std::string parm_nm, int parm_vl)
{
    int indx;
    bool dflt;

    dflt = true;
    for (indx = 0; indx < params->params_count; indx++) {
        if (mystreq(params->params_array[indx].param_name, parm_nm.c_str()) ) {
            dflt = false;
        }
    }
    if (dflt == true) {
        util_parms_add(params, parm_nm.c_str(), std::to_string(parm_vl).c_str());
    }

}

void util_parms_add_default(ctx_params *params, std::string parm_nm, std::string parm_vl)
{
    int indx;
    bool dflt;

    dflt = true;
    for (indx = 0; indx < params->params_count; indx++) {
        if (mystreq(params->params_array[indx].param_name, parm_nm.c_str()) ) {
            dflt = false;
        }
    }
    if (dflt == true) {
        util_parms_add(params, parm_nm.c_str(), parm_vl.c_str());
    }

}

/* Update config line with the values from the params array */
void util_parms_update(ctx_params *params, std::string &confline)
{
    int indx;
    char *tst;
    std::string parmline;

    for (indx = 0; indx < params->params_count; indx++) {
        if (indx == 0) {
            parmline = " ";
        } else {
            parmline += ",";
        }
        tst = strstr(params->params_array[indx].param_name," ");
        if (tst == NULL) {
            parmline += params->params_array[indx].param_name;
        } else {
            parmline += "\"";
            parmline += params->params_array[indx].param_name;
            parmline += "\"";
        }
        parmline += "=";

        tst = strstr(params->params_array[indx].param_value," ");
        if (tst == NULL) {
            parmline += params->params_array[indx].param_value;
        } else {
            parmline += "\"";
            parmline += params->params_array[indx].param_value;
            parmline += "\"";
        }

    }
    parmline += " ";

    confline = parmline;

    MOTPLS_LOG(INF, TYPE_ALL, NO_ERRNO,_("New config: %s"), confline.c_str());

    return;

}

/**
 * util_exec_command
 *      Execute 'command' with 'arg' as its argument.
 *      if !arg command is started with no arguments
 *      Before we call execl we need to close all the file handles
 *      that the fork inherited from the parent in order not to pass
 *      the open handles on to the shell
 */
void util_exec_command(ctx_dev *cam, const char *command, char *filename, int filetype)
{
    char stamp[PATH_MAX];
    timespec tmpts;

    if (cam->current_image == NULL) {
        clock_gettime(CLOCK_REALTIME, &tmpts);
        mystrftime(cam, stamp, sizeof(stamp), command, &tmpts, filename, filetype);
    } else {
        mystrftime(cam, stamp, sizeof(stamp), command, &cam->current_image->imgts, filename, filetype);
    }

    // Tells the OS that we will not be waiting for this forks return code and it can be discarded immediately after it exits
    signal(SIGCHLD,SIG_IGN);
    if (!fork()) {
        /* Detach from parent */
        setsid();

        execl("/bin/sh", "sh", "-c", stamp, " &",(char*)NULL);

        /* if above function succeeds the program never reach here */
        MOTPLS_LOG(ALR, TYPE_EVENTS, SHOW_ERRNO
            ,_("Unable to start external command '%s'"), stamp);

        exit(1);
    }

    MOTPLS_LOG(DBG, TYPE_EVENTS, NO_ERRNO
        ,_("Executing external command '%s'"), stamp);
}

