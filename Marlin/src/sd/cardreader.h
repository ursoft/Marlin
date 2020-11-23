/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "../inc/MarlinConfig.h"

#define IFSD(A,B) TERN(SDSUPPORT,A,B)

#if ENABLED(SDSUPPORT)

#if BOTH(SDCARD_SORT_ALPHA, SDSORT_DYNAMIC_RAM)
  #define SD_RESORT 1
#endif

#if defined(SDSORT_GCODE_DEFAULT_SORT_ALPHA) && true != SDSORT_GCODE_DEFAULT_SORT_ALPHA
  #define SD_ORDER(N,C) (CardReader::getSortAlpha() ? (N) : ((C) - 1 - (N)))
#else
#if ENABLED(SDCARD_RATHERRECENTFIRST) && DISABLED(SDCARD_SORT_ALPHA)
  #define SD_ORDER(N,C) ((C) - 1 - (N))
#else
  #define SD_ORDER(N,C) N
#endif
#endif

#define MAX_DIR_DEPTH     10       // Maximum folder depth
#define MAXDIRNAMELENGTH   8       // DOS folder name size
#define MAXPATHNAMELENGTH  (1 + (MAXDIRNAMELENGTH + 1) * (MAX_DIR_DEPTH) + 1 + FILENAME_LENGTH) // "/" + N * ("ADIRNAME/") + "filename.ext"

#include "SdFile.h"

typedef struct {
  bool saving:1,
       logging:1,
       sdprinting:1,
       mounted:1,
       filenameIsDir:1,
       workDirIsRoot:1,
       abort_sd_printing:1
       #if ENABLED(BINARY_FILE_TRANSFER)
         , binary_mode:1
       #endif
    ;
} card_flags_t;

class CardReader {
public:
  static card_flags_t flag;                         // Flags (above)
  static char filename[FILENAME_LENGTH],            // DOS 8.3 filename of the selected item
              longFilename[LONG_FILENAME_LENGTH];   // Long name of the selected item
#ifdef CYRILLIC_FILENAMES // convert 1251 to  utf8
  static char _longFilenameCyr[LONG_FILENAME_LENGTH*3];
  static const char *longFilenameCyr() {
    static const int table[128] = {                    
        0x82D0,0x83D0,0x9A80E2,0x93D1,0x9E80E2,0xA680E2,0xA080E2,0xA180E2,
        0xAC82E2,0xB080E2,0x89D0,0xB980E2,0x8AD0,0x8CD0,0x8BD0,0x8FD0,    
        0x92D1,0x9880E2,0x9980E2,0x9C80E2,0x9D80E2,0xA280E2,0x9380E2,0x9480E2,
        0,0xA284E2,0x99D1,0xBA80E2,0x9AD1,0x9CD1,0x9BD1,0x9FD1,               
        0xA0C2,0x8ED0,0x9ED1,0x88D0,0xA4C2,0x90D2,0xA6C2,0xA7C2,              
        0x81D0,0xA9C2,0x84D0,0xABC2,0xACC2,0xADC2,0xAEC2,0x87D0,              
        0xB0C2,0xB1C2,0x86D0,0x96D1,0x91D2,0xB5C2,0xB6C2,0xB7C2,              
        0x91D1,0x9684E2,0x94D1,0xBBC2,0x98D1,0x85D0,0x95D1,0x97D1,            
        0x90D0,0x91D0,0x92D0,0x93D0,0x94D0,0x95D0,0x96D0,0x97D0,
        0x98D0,0x99D0,0x9AD0,0x9BD0,0x9CD0,0x9DD0,0x9ED0,0x9FD0,
        0xA0D0,0xA1D0,0xA2D0,0xA3D0,0xA4D0,0xA5D0,0xA6D0,0xA7D0,
        0xA8D0,0xA9D0,0xAAD0,0xABD0,0xACD0,0xADD0,0xAED0,0xAFD0,
        0xB0D0,0xB1D0,0xB2D0,0xB3D0,0xB4D0,0xB5D0,0xB6D0,0xB7D0,
        0xB8D0,0xB9D0,0xBAD0,0xBBD0,0xBCD0,0xBDD0,0xBED0,0xBFD0,
        0x80D1,0x81D1,0x82D1,0x83D1,0x84D1,0x85D1,0x86D1,0x87D1,
        0x88D1,0x89D1,0x8AD1,0x8BD1,0x8CD1,0x8DD1,0x8ED1,0x8FD1
    };
    int n = 0;
    for(int i = 0; i < LONG_FILENAME_LENGTH && n < (int)sizeof(_longFilenameCyr); i++) {
      char c1251 = longFilename[i];
      if(c1251 >= 0 && c1251 <= 0x7f) _longFilenameCyr[n++] = c1251; else {
        int v = table[(int)(0x7f & c1251)];
        _longFilenameCyr[n++] = (char)v;
        _longFilenameCyr[n++] = (char)(v >> 8);
        if (v >>= 16)
            _longFilenameCyr[n++] = (char)v;
      }
      if(c1251 == 0) break;
    }
    _longFilenameCyr[sizeof(_longFilenameCyr) - 1] = '\0';
    return _longFilenameCyr;
  }
#endif

  // Fast! binary file transfer
  #if ENABLED(BINARY_FILE_TRANSFER)
    #if HAS_MULTI_SERIAL
      static int8_t transfer_port_index;
    #else
      static constexpr int8_t transfer_port_index = 0;
    #endif
  #endif

  // // // Methods // // //

  CardReader();

  static SdFile getroot() { return root; }

  static void mount();
  static void release();
#if SD_CONNECTION_IS(LCD_AND_ONBOARD)
  static bool isMountedOnBoard() { return flag.mounted && sd2card.isOnBoard(); }
#endif
  static inline bool isMounted() { return flag.mounted; }
  static void ls();

  // Handle media insert/remove
  static void manage_media();

  // SD Card Logging
  static void openLogFile(char * const path);
  static void write_command(char * const buf);

  // Auto-Start files
  static int8_t autostart_index;                    // Index of autoX.g files
  static void beginautostart();
  static void checkautostart();

  // Basic file ops
  static void openFileRead(char * const path, const uint8_t subcall=0);
  static void openFileWrite(char * const path);
  static void closefile(const bool store_location=false);
  static void removeFile(const char * const name);

  static inline char* longest_filename() { return longFilename[0] ? longFilename : filename; }
  #if ENABLED(LONG_FILENAME_HOST_SUPPORT)
    static void printLongPath(char * const path);   // Used by M33
  #endif

  // Working Directory for SD card menu
  static void cdroot();
  static void cd(const char *relpath);
  static int8_t cdup();
  static uint16_t countFilesInWorkDir();
  static uint16_t get_num_Files();

  // Select a file
  static void selectFileByIndex(const uint16_t nr);
  static void selectFileByName(const char* const match);

  // Print job
  static void openAndPrintFile(const char *name);   // (working directory)
  static void fileHasFinished();
  static void getAbsFilename(char *dst);
  static void printFilename();
  static void startFileprint();
  static void endFilePrint(TERN_(SD_RESORT, const bool re_sort=false));
  static void report_status();
  static inline void pauseSDPrint() { flag.sdprinting = false; }
  static inline bool isPaused() { return isFileOpen() && !flag.sdprinting; }
  static inline bool isPrinting() { return flag.sdprinting; }
  #if HAS_PRINT_PROGRESS_PERMYRIAD
    static inline uint16_t permyriadDone() { return (isFileOpen() && filesize) ? sdpos / ((filesize + 9999) / 10000) : 0; }
  #endif
  static inline uint8_t percentDone() { return (isFileOpen() && filesize) ? sdpos / ((filesize + 99) / 100) : 0; }

  // Helper for open and remove
  static const char* diveToFile(const bool update_cwd, SdFile*& curDir, const char * const path, const bool echo=false);

  #if ENABLED(SDCARD_SORT_ALPHA)
    static void presort();
    static void getfilename_sorted(const uint16_t nr);
    #if ENABLED(SDSORT_GCODE)
      FORCE_INLINE static void setSortOn(bool b) { sort_alpha = b; presort(); }
      FORCE_INLINE static void setSortFolders(int i) { sort_folders = i; presort(); }
      //FORCE_INLINE static void setSortReverse(bool b) { sort_reverse = b; }
    #endif
  #else
    FORCE_INLINE static void getfilename_sorted(const uint16_t nr) { selectFileByIndex(nr); }
  #endif

  #if ENABLED(POWER_LOSS_RECOVERY)
    static bool jobRecoverFileExists();
    static void openJobRecoveryFile(const bool read);
    static void removeJobRecoveryFile();
  #endif

  static inline bool isFileOpen() { return isMounted() && file.isOpen(); }
  static inline uint32_t getIndex() { return sdpos; }
  static inline uint32_t getFileSize() { return filesize; }
  static inline bool eof() { return sdpos >= filesize; }
  static inline void setIndex(const uint32_t index) { sdpos = index; file.seekSet(index); }
  static inline char* getWorkDirName() { workDir.getDosName(filename); return filename; }
  static inline int16_t get() { sdpos = file.curPosition(); return (int16_t)file.read(); }
  static inline int16_t read(void* buf, uint16_t nbyte) { return file.isOpen() ? file.read(buf, nbyte) : -1; }
  static inline int16_t write(void* buf, uint16_t nbyte) { return file.isOpen() ? file.write(buf, nbyte) : -1; }

  static Sd2Card& getSd2Card() { return sd2card; }

  #if ENABLED(AUTO_REPORT_SD_STATUS)
    static void auto_report_sd_status();
    static inline void set_auto_report_interval(uint8_t v) {
      TERN_(HAS_MULTI_SERIAL, auto_report_port = serial_port_index);
      NOMORE(v, 60);
      auto_report_sd_interval = v;
      next_sd_report_ms = millis() + 1000UL * v;
    }
  #endif
  #if ENABLED(LONGCLICK_FOR_IDLE)
    static bool on_event(const char *file_name81);
  #endif
  #if BOTH(SDCARD_SORT_ALPHA,SDSORT_GCODE)
    static bool getSortAlpha() { return sort_alpha; }
  #endif

private:
  //
  // Working directory and parents
  //
  static SdFile root, workDir, workDirParents[MAX_DIR_DEPTH];
  static uint8_t workDirDepth;

  //
  // Alphabetical file and folder sorting
  //
  #if ENABLED(SDCARD_SORT_ALPHA)
    static uint16_t sort_count;   // Count of sorted items in the current directory
    #if ENABLED(SDSORT_GCODE)
      static bool sort_alpha;     // Flag to enable / disable the feature
      static int sort_folders;    // Folder sorting before/none/after
      //static bool sort_reverse; // Flag to enable / disable reverse sorting
    #endif

    // By default the sort index is static
    #if ENABLED(SDSORT_DYNAMIC_RAM)
      static uint8_t *sort_order;
    #else
      static uint8_t sort_order[SDSORT_LIMIT];
    #endif

    #if BOTH(SDSORT_USES_RAM, SDSORT_CACHE_NAMES) && DISABLED(SDSORT_DYNAMIC_RAM)
      #define SORTED_LONGNAME_MAXLEN (SDSORT_CACHE_VFATS) * (FILENAME_LENGTH)
      #define SORTED_LONGNAME_STORAGE (SORTED_LONGNAME_MAXLEN + 1)
    #else
      #define SORTED_LONGNAME_MAXLEN LONG_FILENAME_LENGTH
      #define SORTED_LONGNAME_STORAGE SORTED_LONGNAME_MAXLEN
    #endif

    // Cache filenames to speed up SD menus.
    #if ENABLED(SDSORT_USES_RAM)

      // If using dynamic ram for names, allocate on the heap.
      #if ENABLED(SDSORT_CACHE_NAMES)
        static uint16_t nrFiles; // Cache the total count
        #if ENABLED(SDSORT_DYNAMIC_RAM)
          static char **sortshort, **sortnames;
        #else
          static char sortshort[SDSORT_LIMIT][FILENAME_LENGTH];
        #endif
      #endif

      #if (ENABLED(SDSORT_CACHE_NAMES) && DISABLED(SDSORT_DYNAMIC_RAM)) || NONE(SDSORT_CACHE_NAMES, SDSORT_USES_STACK)
        static char sortnames[SDSORT_LIMIT][SORTED_LONGNAME_STORAGE];
      #endif

      // Folder sorting uses an isDir array when caching items.
      #if HAS_FOLDER_SORTING
        #if ENABLED(SDSORT_DYNAMIC_RAM)
          static uint8_t *isDir;
        #elif ENABLED(SDSORT_CACHE_NAMES) || DISABLED(SDSORT_USES_STACK)
          static uint8_t isDir[(SDSORT_LIMIT+7)>>3];
        #endif
      #endif

    #endif // SDSORT_USES_RAM

  #endif // SDCARD_SORT_ALPHA

  static Sd2Card sd2card;
  static SdVolume volume;
  static SdFile file;

  static uint32_t filesize, sdpos;

  //
  // Procedure calls to other files
  //
  #ifndef SD_PROCEDURE_DEPTH
    #define SD_PROCEDURE_DEPTH 1
  #endif
  static uint8_t file_subcall_ctr;
  static uint32_t filespos[SD_PROCEDURE_DEPTH];
  static char proc_filenames[SD_PROCEDURE_DEPTH][MAXPATHNAMELENGTH];

  //
  // SD Auto Reporting
  //
  #if ENABLED(AUTO_REPORT_SD_STATUS)
    static uint8_t auto_report_sd_interval;
    static millis_t next_sd_report_ms;
    #if HAS_MULTI_SERIAL
      static int8_t auto_report_port;
    #endif
  #endif

  //
  // Directory items
  //
  static bool is_dir_or_gcode(const dir_t &p);
  static int countItems(SdFile dir);
  static void selectByIndex(SdFile dir, const uint8_t index);
  static void selectByName(SdFile dir, const char * const match);
  static void printListing(SdFile parent, const char * const prepend=nullptr);

  #if ENABLED(SDCARD_SORT_ALPHA)
    static void flush_presort();
  #endif
};

#if ENABLED(USB_FLASH_DRIVE_SUPPORT)
  #define IS_SD_INSERTED() Sd2Card::isInserted()
#elif PIN_EXISTS(SD_DETECT)
  #ifdef SD_DETECT_PIN_OB
    #define IS_SD_INSERTED() ((READ(SD_DETECT_PIN)==SD_DETECT_STATE?1:0)+(READ(SD_DETECT_PIN_OB)==SD_DETECT_STATE?2:0))
    #define IS_EXT_SD_INSERTED() (READ(SD_DETECT_PIN) == SD_DETECT_STATE)
  #else
    #define IS_SD_INSERTED() (READ(SD_DETECT_PIN) == SD_DETECT_STATE)
  #endif
#else
  // No card detect line? Assume the card is inserted.
  #define IS_SD_INSERTED() true
#endif

#define IS_SD_PRINTING()  card.flag.sdprinting
#define IS_SD_PAUSED()    card.isPaused()
#define IS_SD_FILE_OPEN() card.isFileOpen()

extern CardReader card;

#else // !SDSUPPORT

#define IS_SD_PRINTING()  false
#define IS_SD_PAUSED()    false
#define IS_SD_FILE_OPEN() false

#define LONG_FILENAME_LENGTH 0

#endif // !SDSUPPORT
