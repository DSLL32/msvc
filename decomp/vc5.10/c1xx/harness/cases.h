/* Native fixture adapters; these are outside the reconstructed App graph. */
#ifndef C1XX_HARNESS_CASES_H
#define C1XX_HARNESS_CASES_H
#include <recon.h>
int c1xx_case_lifecycle(int, char **);
int c1xx_case_minimal_setup(void);
int c1xx_case_minimal(int, char **);
#endif
