#include "order.h"
struct WINEXE_FIELD field1 = { 56, 54, -1, "Sales Order", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Order Number", "ORDERS", "orders","RW",
      "Order Number", "items", "items", "W",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      "Order Number", "ORDERS", "orders",
      "Order Number", "items", "items",
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 0, 3, 0, 7671, 2, 0, 1, 0, 0, 0, 0, 0L, 8912895L, 8388608L, 8912895L, 8388608L, 0,
      { 400, 0, 491, 38 }, { 400, 0, 491, 38 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 3 } ;
struct WINEXE_FIELD field2 = { 57, 54, -1, "Contact Name", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Contact Name", "ORDERS", "orders","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 0, -1, 0, 7671, 3, 0, 1, 0, 0, 0, 0, 0L, 8912895L, 8388608L, 8912895L, 8388608L, 0,
      { 490, 0, 778, 38 }, { 490, 0, 778, 38 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 3 } ;
struct WINEXE_FIELD field3 = { 59, 54, -1, "Phone", "(###)###-####",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Phone", "ORDERS", "orders","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 0, -1, 0, 7671, 4, 0, 1, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 490, 37, 631, 76 }, { 490, 37, 631, 76 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field4 = { 60, 54, -1, "Ext.", "####",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ext", "ORDERS", "orders","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 0, -1, 0, 7671, 5, 0, 1, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 630, 37, 681, 76 }, { 630, 37, 681, 76 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field5 = { 61, 54, -1, "Date", "d-mmm-yy",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Date", "ORDERS", "orders","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      8, -1, 8, -1, 40, 2, 0, -1, 0, 7671, 1, 0, 1, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 680, 37, 778, 76 }, { 680, 37, 778, 76 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field6 = { 68, 54, -1, "Bill Co.", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill Co", "customrs", "customers","RW",
      "Bill Co", "ORDERS", "orders", "RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      "Bill Co", "customrs", "customers",
      NULL, NULL, NULL
      },
      { NULL },
      10, -1, -1, 10, 40, 2, 1, -1, 0, 7671, 6, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 90, 131, 391, 151 }, { 90, 131, 391, 151 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field7 = { 71, 54, -1, "Ship Co.", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship Co", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 1, 7671, 13, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 480, 131, 781, 151 }, { 480, 131, 781, 151 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field8 = { 74, 54, -1, "Bill Dept.", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill Dept", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 1, 7671, 7, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 90, 150, 391, 169 }, { 90, 150, 391, 169 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field9 = { 77, 54, -1, "Ship Dept.", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship Dept", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 1, 7671, 14, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 480, 150, 781, 169 }, { 480, 150, 781, 169 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field10 = { 80, 54, -1, "Bill Street", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill Street", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 1, 7671, 10, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 70, 168, 391, 188 }, { 70, 168, 391, 188 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field11 = { 83, 54, -1, "Ship Street", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship Street", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 1, 7671, 15, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 460, 168, 781, 188 }, { 460, 168, 781, 188 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field12 = { 86, 54, -1, "Bill City", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill City", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 1, 7671, 8, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 50, 187, 391, 207 }, { 50, 187, 391, 207 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field13 = { 89, 54, -1, "Ship City", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship City", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 1, 7671, 16, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 440, 187, 781, 207 }, { 440, 187, 781, 207 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field14 = { 92, 54, -1, "Bill State", "&&",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill State", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 1, -1, 1, 7671, 9, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 60, 206, 161, 226 }, { 60, 206, 161, 226 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field15 = { 94, 54, -1, "Bill Zip", "#####",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill Zip", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 1, -1, 1, 7671, 11, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 230, 206, 291, 226 }, { 230, 206, 291, 226 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field16 = { 95, 54, -1, "Bill Zip Ext.", "-####",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill Zip Ext", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 1, -1, 1, 7671, 12, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 280, 206, 341, 226 }, { 280, 206, 341, 226 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field17 = { 98, 54, -1, "Ship State", "&&",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship State", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 1, -1, 1, 7671, 17, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 450, 206, 551, 226 }, { 450, 206, 551, 226 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field18 = { 100, 54, -1, "Ship Zip", "#####",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship Zip", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 1, -1, 1, 7671, 18, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 620, 206, 681, 226 }, { 620, 206, 681, 226 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field19 = { 101, 54, -1, "Ship Zip Ext", "-####",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship Zip Ext", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 1, -1, 1, 7671, 19, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 670, 206, 731, 226 }, { 670, 206, 731, 226 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field20 = { 103, 54, 102, "Item No.", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "No", "items", "items","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 0, 2, 0, 7671, 42, 0, 1, 0, 0, 0, 0, 0L, 16776960L, 0L, 16777215L, 16777215L, 0,
      { 0, 0, 60, 1 }, { 21, 244, 79, 387 }, { 20, 244, 80, 387 }, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field21 = { 104, 54, 102, "Part No.", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Part No", "items", "items","RW",
      "Part No", "parts", "parts", "RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      "Part No", "parts", "parts",
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 0, 2, 0, 7671, 44, 0, 1, 0, 0, 0, 0, 0L, 16776960L, 0L, 16777215L, 16777215L, 0,
      { 0, 0, 60, 1 }, { 81, 244, 139, 387 }, { 80, 244, 140, 387 }, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field22 = { 105, 54, 102, "Description", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Description", "parts", "parts","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 0, -1, 0, 7671, 43, 0, 1, 0, 0, 0, 0, 0L, 16776960L, 0L, 16777215L, 16777215L, 0,
      { 0, 0, 260, 1 }, { 141, 244, 399, 387 }, { 140, 244, 400, 387 }, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field23 = { 106, 54, 102, "Qty.", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Qty", "items", "items","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 0, 2, 0, 7671, 45, 0, 1, 0, 0, 0, 0, 0L, 16776960L, 0L, 16777215L, 16777215L, 0,
      { 0, 0, 60, 1 }, { 401, 244, 459, 387 }, { 400, 244, 460, 387 }, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field24 = { 107, 54, 102, "Price", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Price", "parts", "parts","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      4, 0, -1, -1, 10, 2, 0, 2, 0, 7671, 46, 0, 1, 0, 0, 0, 0, 0L, 16776960L, 0L, 16777215L, 16777215L, 0,
      { 0, 0, 100, 1 }, { 461, 244, 559, 387 }, { 460, 244, 560, 387 }, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field25 = { 108, 54, 102, "Discount", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "", "", "","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      5, 0, -1, -1, 10, 0, 0, 2, 0, 7671, 47, 0, 1, 0, 0, 0, 0, 0L, 16776960L, 0L, 16777215L, 16777215L, 0,
      { 0, 0, 70, 1 }, { 561, 244, 629, 387 }, { 560, 244, 630, 387 }, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field26 = { 109, 54, 102, "Amount", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Amount", "items", "items","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      4, 0, -1, -1, 10, 2, 0, 2, 0, 7671, 48, 0, 1, 0, 0, 0, 0, 0L, 16776960L, 0L, 16777215L, 16777215L, 0,
      { 0, 0, 140, 1 }, { 631, 244, 769, 387 }, { 630, 244, 770, 387 }, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field27 = { 111, 54, -1, "Subtotal", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "", "", "","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, 2, 0, 7671, 21, 0, 1, 0, 0, 0, 0, 0L, 8912895L, 0L, 8912895L, 0L, 0,
      { 630, 393, 771, 413 }, { 630, 393, 771, 413 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field28 = { 113, 54, -1, "Tax", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "", "", "","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, 2, 0, 7671, 22, 0, 1, 0, 0, 0, 0, 0L, 8912895L, 0L, 8912895L, 0L, 0,
      { 630, 412, 771, 432 }, { 630, 412, 771, 432 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field29 = { 115, 54, -1, "Shipping", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "", "", "","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, 2, 0, 7671, 23, 0, 1, 0, 0, 0, 0, 0L, 8912895L, 0L, 8912895L, 0L, 0,
      { 630, 431, 771, 451 }, { 630, 431, 771, 451 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field30 = { 121, 54, -1, "Total", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "", "", "","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, 2, 0, 7671, 24, 0, 0, 0, 1, 1, 1, 0L, 8912895L, 0L, 8912895L, 0L, 0,
      { 630, 459, 771, 479 }, { 630, 459, 771, 479 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field31 = { 126, 122, -1, "Bill Co.", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill Co", "customrs", "customers","RW",
      "Bill Co", "ORDERS", "orders", "RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      "Bill Co", "customrs", "customers",
      NULL, NULL, NULL
      },
      { NULL },
      11, 0, -1, 11, 40, 2, 1, -1, 0, 7671, 6, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 100, 18, 401, 38 }, { 100, 18, 401, 38 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field32 = { 129, 122, -1, "Bill Dept.", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill Dept", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 0, 7671, 7, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 100, 37, 401, 57 }, { 100, 37, 401, 57 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field33 = { 132, 122, -1, "Bill Street", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill Street", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 0, 7671, 10, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 80, 56, 401, 76 }, { 80, 56, 401, 76 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field34 = { 135, 122, -1, "Bill City", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill City", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 0, 7671, 8, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 60, 75, 401, 94 }, { 60, 75, 401, 94 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field35 = { 138, 122, -1, "Bill State", "&&",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill State", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 1, -1, 0, 7671, 9, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 70, 93, 171, 113 }, { 70, 93, 171, 113 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field36 = { 140, 122, -1, "Bill Zip", "#####",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill Zip", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 1, -1, 0, 7671, 11, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 240, 93, 301, 113 }, { 240, 93, 301, 113 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field37 = { 141, 122, -1, "Bill Zip Ext.", "-####",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Bill Zip Ext", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 1, -1, 0, 7671, 12, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 290, 93, 351, 113 }, { 290, 93, 351, 113 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field38 = { 146, 122, -1, "Ship Co.", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship Co", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 0, 7671, 13, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 100, 150, 401, 169 }, { 100, 150, 401, 169 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field39 = { 149, 122, -1, "Ship Dept.", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship Dept", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 0, 7671, 14, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 100, 168, 401, 188 }, { 100, 168, 401, 188 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field40 = { 152, 122, -1, "Ship Street", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship Street", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 0, 7671, 15, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 80, 187, 401, 207 }, { 80, 187, 401, 207 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field41 = { 155, 122, -1, "Ship City", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship City", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 1, -1, 0, 7671, 16, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 60, 206, 401, 226 }, { 60, 206, 401, 226 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field42 = { 158, 122, -1, "Ship State", "&&",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship State", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 1, -1, 0, 7671, 17, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 70, 225, 171, 244 }, { 70, 225, 171, 244 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field43 = { 160, 122, -1, "Ship Zip", "#####",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship Zip", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 1, -1, 0, 7671, 18, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 240, 225, 301, 244 }, { 240, 225, 301, 244 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field44 = { 161, 122, -1, "Ship Zip Ext", "-####",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Ship Zip Ext", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      2, 2, -1, -1, 40, 2, 1, -1, 0, 7671, 19, 0, 0, 0, 0, 0, 0, 0L, 16777215L, 0L, 16777215L, 0L, 1,
      { 290, 225, 351, 244 }, { 290, 225, 351, 244 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field45 = { 163, 122, -1, "Customer Type", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Customer Type", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      13, -1, -1, 13, 40, 2, 0, -1, 0, 7671, 35, 0, 0, 0, 0, 0, 0, 0L, 8912895L, 0L, 8912895L, 8421504L, 0,
      { 30, 262, 431, 310 }, { 30, 262, 431, 310 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field46 = { 164, 122, -1, "Tax Exempt", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Tax Exempt", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      14, -1, -1, 14, 40, 2, 0, -1, 0, 7671, 20, 0, 0, 1, 0, 0, 0, 0L, 8912895L, 0L, 8912895L, 0L, 0,
      { 30, 309, 151, 347 }, { 30, 309, 151, 347 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;
struct WINEXE_FIELD field47 = { 165, 122, -1, "Tax Exempt No.", "",
      {  /* READ/WRITE fieldname, filename, ov_linkname, RW */
      "Tax Exempt No", "customrs", "customers","RW",
      NULL, NULL, NULL, NULL
      },
      { /* LOCATE fieldname, filename, ov_linkname */
      NULL, NULL, NULL
      },
      { NULL },
      0, 0, -1, -1, 40, 2, 0, -1, 0, 7671, 36, 0, 0, 1, 1, 0, 0, 0L, 8912895L, 0L, 8912895L, 0L, 0,
      { 150, 309, 431, 347 }, { 150, 309, 431, 347 }, {0, 0, 0, 0}, NULL, NULL, NULL, NULL, NULL,1, 1 } ;

struct WINEXE_FIELD far * far fields[] = {
   &field1,
   &field2,
   &field3,
   &field4,
   &field5,
   &field6,
   &field7,
   &field8,
   &field9,
   &field10,
   &field11,
   &field12,
   &field13,
   &field14,
   &field15,
   &field16,
   &field17,
   &field18,
   &field19,
   &field20,
   &field21,
   &field22,
   &field23,
   &field24,
   &field25,
   &field26,
   &field27,
   &field28,
   &field29,
   &field30,
   &field31,
   &field32,
   &field33,
   &field34,
   &field35,
   &field36,
   &field37,
   &field38,
   &field39,
   &field40,
   &field41,
   &field42,
   &field43,
   &field44,
   &field45,
   &field46,
   &field47,
   NULL
   } ;
