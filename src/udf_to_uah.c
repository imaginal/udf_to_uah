/*
   Copyright (c) 2018, Dozorro.org

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/*
**
** After the library is made one must notify mysqld about the new
** functions with the commands:
**
** CREATE FUNCTION to_uah RETURNS REAL SONAME "udf_to_uah.so";
**
** After this the functions will work exactly like native MySQL functions.
** Functions should be created only once.
**
** The functions can be deleted by:
**
** DROP FUNCTION to_uah;
**
** The CREATE FUNCTION and DROP FUNCTION update the func@mysql table. All
** Active function will be reloaded on every restart of server
** (if --skip-grant-tables is not given)
**
** If you ge problems with undefined symbols when loading the shared
** library, you should verify that mysqld is compiled with the -rdynamic
** option.
**
** If you can't get AGGREGATES to work, check that you have the column
** 'type' in the mysql.func table.  If not, run 'mysql_upgrade'.
**
*/

#ifdef STANDARD
/* STANDARD is defined, don't use any mysql functions */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __WIN__
typedef unsigned __int64 ulonglong;	/* Microsofts 64 bit types */
typedef __int64 longlong;
#else
typedef unsigned long long ulonglong;
typedef long long longlong;
#endif /*__WIN__*/
#else
#include <my_global.h>
#include <my_sys.h>
#if defined(MYSQL_SERVER)
#include <m_string.h>		/* To get strmov() */
#else
/* when compiled as standalone */
#include <string.h>
#define strmov(a,b) stpcpy(a,b)
#endif
#endif
#include <mysql.h>
#include <ctype.h>

#ifdef _WIN32
/* inet_aton needs winsock library */
#pragma comment(lib, "ws2_32")
#endif

#ifdef HAVE_DLOPEN

#if !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_SOLARIS_STYLE_GETHOST)
static pthread_mutex_t LOCK_hostname;
#endif

/* These must be right or mysqld will not find the symbol! */

my_bool to_uah_init(UDF_INIT *, UDF_ARGS *args, char *message);
void to_uah_deinit(UDF_INIT *initid);
double to_uah(UDF_INIT *initid, UDF_ARGS *args, char *is_null,
		     char *error);


/*************************************************************************
** Example of init function
** Arguments:
** initid	Points to a structure that the init function should fill.
**		This argument is given to all other functions.
**	my_bool maybe_null	1 if function can return NULL
**				Default value is 1 if any of the arguments
**				is declared maybe_null.
**	unsigned int decimals	Number of decimals.
**				Default value is max decimals in any of the
**				arguments.
**	unsigned int max_length  Length of string result.
**				The default value for integer functions is 21
**				The default value for real functions is 13+
**				default number of decimals.
**				The default value for string functions is
**				the longest string argument.
**	char *ptr;		A pointer that the function can use.
**
** args		Points to a structure which contains:
**	unsigned int arg_count		Number of arguments
**	enum Item_result *arg_type	Types for each argument.
**					Types are STRING_RESULT, REAL_RESULT
**					and INT_RESULT.
**	char **args			Pointer to constant arguments.
**					Contains 0 for not constant argument.
**	unsigned long *lengths;		max string length for each argument
**	char *maybe_null		Information of which arguments
**					may be NULL
**
** message	Error message that should be passed to the user on fail.
**		The message buffer is MYSQL_ERRMSG_SIZE big, but one should
**		try to keep the error message less than 80 bytes long!
**
** This function should return 1 if something goes wrong. In this case
** message should contain something useful!
**************************************************************************/

#define CURR_MAX_LEN  20
#define DATE_MAX_LEN  40
#define DATE_KEY_LEN  10

my_bool to_uah_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
  if (!args->arg_count)
  {
    strcpy(message,"TO_UAH must have at least one argument");
    return 1;
  }
  args->arg_type[0] = REAL_RESULT;
  if (args->arg_count > 1)
	  args->arg_type[1] = STRING_RESULT;
  if (args->arg_count > 2)
	  args->arg_type[2] = STRING_RESULT;
  initid->maybe_null = 1;		/* The result may be null */
  initid->decimals = 2;		/* We want 2 decimals in the result */
  initid->max_length = 14;		/* 11 digits + . + 2 decimals */
  return 0;
}


/****************************************************************************
** Deinit function. This should free all resources allocated by
** this function.
** Arguments:
** initid	Return value from xxxx_init
****************************************************************************/

void to_uah_deinit(UDF_INIT *initid __attribute__((unused)))
{
}


/***************************************************************************
** UDF double function.
** Arguments:
** initid	Structure filled by xxx_init
** args		The same structure as to xxx_init. This structure
**		contains values for all parameters.
**		Note that the functions MUST check and convert all
**		to the type it wants!  Null values are represented by
**		a NULL pointer
** is_null	If the result is null, one should store 1 here.
** error	If something goes fatally wrong one should store 1 here.
**
** This function should return the result.
***************************************************************************/

double to_uah(UDF_INIT *initid __attribute__((unused)), UDF_ARGS *args,
                     char *is_null, char *error __attribute__((unused)))
{
  int i = 0;
  double val = 0.0;
  char currency[CURR_MAX_LEN+1] = {0};
  char ondate[DATE_MAX_LEN+1] = {0};

  if (!args->arg_count || !args || !args->args[0])
  {
	  *is_null=1;
	  return 0.0;
  }

  switch (args->arg_type[i]) {
  case STRING_RESULT:			/* Add string lengths */
    if (args->lengths[i])
    	val = atof(args->args[i]);
    break;
  case INT_RESULT:			/* Add numbers */
    val = (double) *((longlong*) args->args[i]);
    break;
  case REAL_RESULT:			/* Add numers as longlong */
    val = *((double*) args->args[i]);
    break;
  default:
    *is_null=1;
    return 0.0;
  }

  if (args->arg_count > 1 && args->arg_type[1] == STRING_RESULT
    && args->lengths[1] >= 1 && args->lengths[1] <= CURR_MAX_LEN)
  {
  	for (i = 0; i < args->lengths[1] && i < CURR_MAX_LEN; i++)
  		currency[i] = toupper(args->args[1][i]);
    currency[i] = 0;
  }
  if (args->arg_count > 2 && args->arg_type[2] == STRING_RESULT
    && args->lengths[2] >= 1 && args->lengths[2] <= DATE_MAX_LEN)
  {
    // copy only first key_len chars
    for (i = 0; i < args->lengths[2] && i < DATE_KEY_LEN; i++)
      ondate[i] = args->args[2][i];
    ondate[i] = 0;
  }

  if (!currency[0])
  	return val;

  if (strncmp(currency, "USD", 3) == 0)
  	val *= 26;
  else if (strncmp(currency, "EUR", 3) == 0)
  	val *= 30;

  return val;
}


#endif /* HAVE_DLOPEN */
