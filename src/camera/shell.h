/*!
*****************************************************************************
** \file        adi/test/src/shell.c
**
** \brief       shell command fcuntion
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef _SHELL_H_
#define _SHELL_H_

#define SHELL_CLIENT_START         0x1
#define SHELL_SERVER_START         0x2
#define SHELL_SCREEN_START         0x4


typedef enum {
    PARAMS_ERROR = 0,
    PARAMS_CHAR,
    PARAMS_UCHAR,
    PARAMS_SCHAR,
    PARAMS_SHORT,
    PARAMS_USHORT,
    PARAMS_INT,
    PARAMS_UINT,
    PARAMS_STRING,
    PARAMS_FLOAT,
    PARAMS_DOUBLE,
    PARAMS_ENUM,
} PARAMS_TYPE;

#define NAME_TO_STRING(n)  #n

typedef struct params_s {
    char *name;
    void *addr;
    PARAMS_TYPE type;
    char *hint;
} PARAMS_STRCUT;

#define GET_TYPE(p)  __builtin_choose_expr(  \
        __builtin_types_compatible_p(typeof(p), char),  \
        PARAMS_CHAR,    \
         __builtin_choose_expr(  \
        __builtin_types_compatible_p(typeof(p), unsigned char),  \
        PARAMS_UCHAR,    \
         __builtin_choose_expr(  \
        __builtin_types_compatible_p(typeof(p), signed char),  \
        PARAMS_SCHAR,    \
        __builtin_choose_expr(  \
        __builtin_types_compatible_p(typeof(p), short),  \
        PARAMS_SHORT,    \
        __builtin_choose_expr(  \
        __builtin_types_compatible_p(typeof(p), unsigned short),  \
        PARAMS_USHORT,    \
        __builtin_choose_expr(  \
        __builtin_types_compatible_p(typeof(p), int),  \
        PARAMS_INT,    \
        __builtin_choose_expr(  \
        __builtin_types_compatible_p(typeof(p), unsigned int),  \
        PARAMS_UINT,    \
        __builtin_choose_expr(  \
        __builtin_types_compatible_p(typeof(p), char *),  \
        PARAMS_STRING,    \
        __builtin_choose_expr(  \
        __builtin_types_compatible_p(typeof(p), float),  \
        PARAMS_FLOAT,    \
        __builtin_choose_expr(  \
        __builtin_types_compatible_p(typeof(p), double),  \
        PARAMS_DOUBLE,    \
        PARAMS_ENUM))))))))))
/* create one input menu on shell */
#define CREATE_INPUT_MENU(n) \
        { \
            const char *s_name = NAME_TO_STRING(n); \
            PARAMS_STRCUT sub_params[] =
/* add submenu , use get input data. */
#define ADD_SUBMENU(sub, hint) {NAME_TO_STRING(sub), &(sub), GET_TYPE(sub), (hint)}

/* create input menu complete, start enter menu. */
#define CREATE_INPUT_MENU_COMPLETE()     {NULL, NULL, PARAMS_ERROR}}
#define DISPLAY_MENU()    shell_get_param_by_menu(s_name, sub_params, \
                         sizeof(sub_params)/sizeof(typeof(sub_params[0])) - 1)
/*
{ 
            const char *s_name = NAME_TO_STRING(n); 
            PARAMS_STRCUT sub_params[] =

#define ADD_SUBMENU(sub, hint) {NAME_TO_STRING(sub), &(sub), GET_TYPE(sub), (hint)}


#define CREATE_INPUT_MENU_COMPLETE()     {NULL, NULL, PARAMS_ERROR}
}



CREATE_INPUT_MENU(framerate) {
				const char *s_name = NAME_TO_STRING(framerate);
				PARAMS_STRCUT sub_params[] =
				{NAME_TO_STRING(framerate.fps), &(framerate.fps), GET_TYPE(framerate.fps), ("fps")}
                {NULL, NULL, PARAMS_ERROR}}
                if (shell_get_param_by_menu(s_name, sub_params, sizeof(sub_params)/sizeof(typeof(sub_params[0])) - 1) != 0) {
                    GADI_ERROR("get parameter failed!\n");
                    break;
                }
            }

*/
#ifdef __cplusplus
extern "C" {
#endif

/*!
*******************************************************************************
** \brief init shell command  function
**  it provide the command register and run the function from input string
**  it also provide command list for direct post to shell module for execute the command.
**
** \param[in]  priority     shell module priority
**
**
**
** \sa shell_Initation
*******************************************************************************
*/

void shell_init(int priority, int is_client);

/*!
*******************************************************************************
** \brief register command function for "help"
**
** \param[in]  argc A counter that holds the number of strings stored in argv[].
** \param[out] argv A string array contains the command itself and all additional
**             parameters (strings).
**
** \return
** - # >0             command execute success
** - # <=0            command execute failed
**
** \sa shell_helpcommand
*******************************************************************************
*/

int shell_helpcommand(int argc, char* argv[]);

/*!
*******************************************************************************
** \brief register command function for "history"
**
** \param[in]  argc A counter that holds the number of strings stored in argv[].
** \param[out] argv A string array contains the command itself and all additional
**             parameters (strings).
**
** \return
** - # >0             command execute success
** - # <=0            command execute failed
**
** \sa shell_historycommand
*******************************************************************************
*/
int shell_historycommand(int argc, char* argv[]);

/*!
*******************************************************************************
** \brief register command function for "redo"
**
** \param[in]  argc A counter that holds the number of strings stored in argv[].
** \param[out] argv A string array contains the command itself and all additional
**             parameters (strings).
**
** \return
** - # >0             command execute success
** - # <=0            command execute failed
**
** \sa shell_redocommand
*******************************************************************************
*/
int shell_redocommand(int argc, char* argv[]);



/*!
*******************************************************************************
** \brief Register a new command.
**
** This function registers the given command to the internal command table.
** An already existing command will be unregistered first and replaced by the
** given parameters. The buffers for the brief and the description texts will
** be dynamically allocated to avoid buffer overflows.
**
** \param[in]  command      The command (string) to register
** \param[in]  function     A short (one line) help message
** \param[in]  brief        The full command description, maybe NULL
** \param[in]  description  The function to be called when the user enters the
**                          "command" (with optional parameters), this functions
**                          must accept the argc, argv argument pair for additional
**                          optional parameters [strings]
**
**
** \return
** - # 0             command register success
** - # !0            command register failed
**
** \sa shell_registercommand
*******************************************************************************
*/
int shell_registercommand(char   *command,
                               int(*function)(int, char**),
                               char   *brief,
                               char   *description);


/*!
*******************************************************************************
** \brief post a command and parameter string to shell module for exectue the register's fcuntion.
**
**
** \param[in]  command      The command and parameter (string) for execute
**
**
** \return
** - # 0             command post success
** - # !0            command post failed
**
** \sa shell_postcommand
*******************************************************************************
*/
int shell_get_strbysys(const char *cmd, char *buff, int bufsize);

int shell_system(const char *cmd);

int shell_get_param_by_menu(const char* s_name, PARAMS_STRCUT *sub_params, int size);

#ifdef __cplusplus
}
#endif

#endif /* _SHELL_H_ */


