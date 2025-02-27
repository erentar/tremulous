/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2013 Darklegion Development
Copyright (C) 2015-2019 GrangerHub

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, see <https://www.gnu.org/licenses/>

===========================================================================
*/
// cvar.c -- dynamic variable tracking

#include "cvar.h"

#include "cmd.h"
#include "files.h"
#include "q_shared.h"
#include "qcommon.h"

static cvar_t *cvar_vars = nullptr;
cvar_t *cvar_cheats;
int cvar_modifiedFlags = 0;

#define MAX_CVARS 2048
static cvar_t cvar_indexes[MAX_CVARS];
static int cvar_numIndexes;

#define FILE_HASH_SIZE 256
static cvar_t *hashTable[FILE_HASH_SIZE];

/*
================
return a hash value for the filename
================
*/
static long generateHashValue(const char *fname)
{
    long hash = 0;
    int i = 0;
    while (fname[i] != '\0')
    {
        char letter = tolower(fname[i]);
        hash += (long)(letter) * (i + 119);
        i++;
    }
    hash &= (FILE_HASH_SIZE - 1);
    return hash;
}

/*
============
Cvar_ValidateString
============
*/
bool Cvar_ValidateString(const char *s)
{
    if (!s)
    {
        return false;
    }
    if (strchr(s, '\\'))
    {
        return false;
    }
    if (strchr(s, '\"'))
    {
        return false;
    }
    if (strchr(s, ';'))
    {
        return false;
    }
    return true;
}

/*
============
Cvar_FindVar
============
*/
cvar_t *Cvar_FindVar(const char *var_name)
{
    long hash = generateHashValue(var_name);

    for (cvar_t *var = hashTable[hash]; var; var = var->hashNext)
        if (!Q_stricmp(var_name, var->name))
            return var;

    return nullptr;
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue(const char *var_name)
{
    cvar_t *var = Cvar_FindVar(var_name);
    if (!var)
        return 0;
    return var->value;
}

/*
============
Cvar_VariableIntegerValue
============
*/
int Cvar_VariableIntegerValue(const char *var_name)
{
    cvar_t *var = Cvar_FindVar(var_name);
    if (!var)
        return 0;
    return var->integer;
}

/*
============
Cvar_VariableString
============
*/
const char *Cvar_VariableString(const char *var_name)
{
    cvar_t *var = Cvar_FindVar(var_name);
    if (!var)
        return "";
    return var->string;
}

/*
============
Cvar_VariableStringBuffer
============
*/
void Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize)
{
    cvar_t *var = Cvar_FindVar(var_name);
    if (var)
        Q_strncpyz(buffer, var->string, bufsize);
    else
        *buffer = 0;
}

/*
============
Cvar_Flags
============
*/
unsigned int Cvar_Flags(const char *var_name)
{
    cvar_t *var;

    if (!(var = Cvar_FindVar(var_name)))
        return CVAR_NONEXISTENT;
    else if (var->modified)
        return var->flags | CVAR_MODIFIED;

    return var->flags;
}

/*
============
Cvar_CommandCompletion
============
*/
void Cvar_CommandCompletion(void (*callback)(const char *s))
{
    for (cvar_t *cvar = cvar_vars; cvar; cvar = cvar->next)
    {
        if (cvar->name)
            callback(cvar->name);
    }
}

/*
============
Cvar_Validate
============
*/
const char *Cvar_Validate(cvar_t *var, const char *value, bool warn)
{
    static char s[MAX_CVAR_VALUE_STRING];
    float valuef;
    bool changed = false;

    if (!var->validate)
        return value;

    if (!value)
        return nullptr;

    if (Q_isanumber(value))
    {
        valuef = atof(value);

        if (var->integral)
        {
            if (!Q_isintegral(valuef))
            {
                if (warn)
                    Com_Printf("WARNING: cvar '%s' must be integral", var->name);

                valuef = (int)valuef;
                changed = true;
            }
        }
    }
    else
    {
        if (warn)
            Com_Printf("WARNING: cvar '%s' must be numeric", var->name);

        valuef = atof(var->resetString);
        changed = true;
    }

    if (valuef < var->min)
    {
        if (warn)
        {
            if (changed)
                Com_Printf(" and is");
            else
                Com_Printf("WARNING: cvar '%s'", var->name);

            if (Q_isintegral(var->min))
                Com_Printf(" out of range (min %d)", (int)var->min);
            else
                Com_Printf(" out of range (min %f)", var->min);
        }

        valuef = var->min;
        changed = true;
    }
    else if (valuef > var->max)
    {
        if (warn)
        {
            if (changed)
                Com_Printf(" and is");
            else
                Com_Printf("WARNING: cvar '%s'", var->name);

            if (Q_isintegral(var->max))
                Com_Printf(" out of range (max %d)", (int)var->max);
            else
                Com_Printf(" out of range (max %f)", var->max);
        }

        valuef = var->max;
        changed = true;
    }

    if (changed)
    {
        if (Q_isintegral(valuef))
        {
            Com_sprintf(s, sizeof(s), "%d", (int)valuef);

            if (warn)
                Com_Printf(", setting to %d\n", (int)valuef);
        }
        else
        {
            Com_sprintf(s, sizeof(s), "%f", valuef);

            if (warn)
                Com_Printf(", setting to %f\n", valuef);
        }

        return s;
    }

    return value;
}

/*
============
Cvar_Get

If the variable already exists, the value will not be set unless CVAR_ROM
The flags will be or'ed in if the variable exists.
============
*/
cvar_t *Cvar_Get(const char *var_name, const char *var_value, int flags)
{
    if (!var_name || !var_value)
    {
        Com_Error(ERR_FATAL, "Cvar_Get: nullptr parameter");
    }

    if (!Cvar_ValidateString(var_name))
    {
        Com_Printf("invalid cvar name string: %s\n", var_name);
        var_name = "BADNAME";
    }

#if 0  // FIXME: values with backslash happen
	if ( !Cvar_ValidateString( var_value ) ) {
		Com_Printf("invalid cvar value string: %s\n", var_value );
		var_value = "BADVALUE";
	}
#endif

    cvar_t *var = Cvar_FindVar(var_name);
    if (var)
    {
        var_value = Cvar_Validate(var, var_value, false);

        // Make sure the game code cannot mark engine-added variables as gamecode vars
        if (var->flags & CVAR_VM_CREATED)
        {
            if (!(flags & CVAR_VM_CREATED))
                var->flags &= ~CVAR_VM_CREATED;
        }
        else if (!(var->flags & CVAR_USER_CREATED))
        {
            if (flags & CVAR_VM_CREATED)
                flags &= ~CVAR_VM_CREATED;
        }

        // if the C code is now specifying a variable that the user already
        // set a value for, take the new value as the reset value
        if (var->flags & CVAR_USER_CREATED)
        {
            var->flags &= ~CVAR_USER_CREATED;
            Z_Free(var->resetString);
            var->resetString = CopyString(var_value);

            if (flags & CVAR_ROM)
            {
                // this variable was set by the user,
                // so force it to value given by the engine.

                if (var->latchedString)
                    Z_Free(var->latchedString);

                var->latchedString = CopyString(var_value);
            }
        }

        // Make sure servers cannot mark engine-added variables as SERVER_CREATED
        if (var->flags & CVAR_SERVER_CREATED)
        {
            if (!(flags & CVAR_SERVER_CREATED))
                var->flags &= ~CVAR_SERVER_CREATED;
        }
        else
        {
            if (flags & CVAR_SERVER_CREATED)
                flags &= ~CVAR_SERVER_CREATED;
        }

        var->flags |= flags;

        // only allow one non-empty reset string without a warning
        if (!var->resetString[0])
        {
            // we don't have a reset string yet
            Z_Free(var->resetString);
            var->resetString = CopyString(var_value);
        }
        else if (var_value[0] && strcmp(var->resetString, var_value))
        {
            Com_DPrintf("Warning: cvar \"%s\" given initial values: \"%s\" and \"%s\"\n",
                    var_name, var->resetString, var_value);
        }
        // if we have a latched string, take that value now
        if (var->latchedString)
        {
            char *s = var->latchedString;
            var->latchedString = nullptr;  // otherwise cvar_set2 would free it
            Cvar_Set2(var_name, s, true);
            Z_Free(s);
        }

        // ZOID--needs to be set so that cvars the game sets as
        // SERVERINFO get sent to clients
        cvar_modifiedFlags |= flags;
        if (flags & CVAR_ALTERNATE_SYSTEMINFO)
        {
            cvar_modifiedFlags |= CVAR_SYSTEMINFO;
        }

        return var;
    }

    //
    // allocate a new cvar
    //

    // find a free cvar
    int i;
    for (i = 0; i < MAX_CVARS; i++)
        if (!cvar_indexes[i].name)
            break;

    if (i >= MAX_CVARS)
    {
        if (!com_errorEntered)
            Com_Error(ERR_FATAL, "Error: Too many cvars, cannot create a new one!");
        return nullptr;
    }

    var = &cvar_indexes[i];

    if (i >= cvar_numIndexes)
        cvar_numIndexes = i + 1;

    var->name = CopyString(var_name);
    var->string = CopyString(var_value);
    var->modified = true;
    var->modificationCount = 1;
    var->value = atof(var->string);
    var->integer = atoi(var->string);
    var->resetString = CopyString(var_value);
    var->validate = false;
    var->description = nullptr;

    // link the variable in
    var->next = cvar_vars;
    if (cvar_vars)
        cvar_vars->prev = var;

    var->prev = nullptr;
    cvar_vars = var;

    var->flags = flags;
    // note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
    cvar_modifiedFlags |= var->flags;
    if (var->flags & CVAR_ALTERNATE_SYSTEMINFO)
        cvar_modifiedFlags |= CVAR_SYSTEMINFO;

    long hash = generateHashValue(var_name);
    var->hashIndex = hash;

    var->hashNext = hashTable[hash];
    if (hashTable[hash])
        hashTable[hash]->hashPrev = var;

    var->hashPrev = nullptr;
    hashTable[hash] = var;

    return var;
}

/*
============
Cvar_Print

Prints the value, default, and latched string of the given variable
============
*/
void Cvar_Print(cvar_t *v)
{
    Com_Printf("\"%s\" is:\"%s" S_COLOR_WHITE "\"", v->name, v->string);

    if (!(v->flags & CVAR_ROM))
    {
        if (!Q_stricmp(v->string, v->resetString))
            Com_Printf(", the default");
        else
            Com_Printf(" default:\"%s" S_COLOR_WHITE "\"", v->resetString);
    }

    Com_Printf("\n");

    if (v->latchedString)
        Com_Printf("latched: \"%s\"\n", v->latchedString);

    if (v->description)
        Com_Printf("%s\n", v->description);
}

/*
============
Cvar_Set2
============
*/
cvar_t *Cvar_Set2(const char *var_name, const char *value, bool force)
{
    if (!Cvar_ValidateString(var_name))
    {
        Com_Printf("invalid cvar name string: %s\n", var_name);
        var_name = "BADNAME";
    }

#if 0  // FIXME
	if ( value && !Cvar_ValidateString( value ) ) {
		Com_Printf("invalid cvar value string: %s\n", value );
		var_value = "BADVALUE";
	}
#endif

    cvar_t *var = Cvar_FindVar(var_name);
    if (!var)
    {
        if (!value)
            return nullptr;

        if (!force)
            return Cvar_Get(var_name, value, CVAR_USER_CREATED);

        return Cvar_Get(var_name, value, 0);
    }

    if (!value)
        value = var->resetString;

    value = Cvar_Validate(var, value, true);

    if ((var->flags & CVAR_LATCH) && var->latchedString)
    {
        if (!strcmp(value, var->string))
        {
            Z_Free(var->latchedString);
            var->latchedString = nullptr;
            return var;
        }

        if (!strcmp(value, var->latchedString))
            return var;
    }
    else if (!strcmp(value, var->string))
        return var;

    // note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
    cvar_modifiedFlags |= var->flags;
    if (var->flags & CVAR_ALTERNATE_SYSTEMINFO)
    {
        cvar_modifiedFlags |= CVAR_SYSTEMINFO;
    }

    if (!force)
    {
        if (var->flags & CVAR_ROM)
        {
            Com_Printf("%s is read only.\n", var_name);
            return var;
        }

        if (var->flags & CVAR_INIT)
        {
            Com_Printf("%s is write protected.\n", var_name);
            return var;
        }

        if (var->flags & CVAR_LATCH)
        {
            if (var->latchedString)
            {
                if (strcmp(value, var->latchedString) == 0)
                    return var;
                Z_Free(var->latchedString);
                var->latchedString = nullptr;
            }
            else
            {
                if (strcmp(value, var->string) == 0)
                    return var;
            }

            Com_Printf("%s will be changed upon restarting.\n", var_name);
            var->latchedString = CopyString(value);
            var->modified = true;
            var->modificationCount++;
            return var;
        }

        if ((var->flags & CVAR_CHEAT) && !cvar_cheats->integer)
        {
            Com_Printf("%s is cheat protected.\n", var_name);
            return var;
        }
    }
    else
    {
        if (var->latchedString)
        {
            Z_Free(var->latchedString);
            var->latchedString = nullptr;
        }
    }

    if (!strcmp(value, var->string))
        return var;  // not changed

    var->modified = true;
    var->modificationCount++;

    Z_Free(var->string);  // free the old value string

    var->string = CopyString(value);
    var->value = atof(var->string);
    var->integer = atoi(var->string);

    return var;
}

/*
============
Cvar_Set
============
*/
void Cvar_Set(const char *var_name, const char *value)
{
    Cvar_Set2(var_name, value, true);
}
/*
============
Cvar_SetSafe
============
*/
void Cvar_SetSafe(const char *var_name, const char *value)
{
    unsigned flags = Cvar_Flags(var_name);

    if ((flags != CVAR_NONEXISTENT) && (flags & CVAR_PROTECTED))
    {
        if (value)
            Com_Error(ERR_DROP, "Restricted source tried to set \"%s\" to \"%s\"",
                var_name, value);
        else
            Com_Error(ERR_DROP, "Restricted source tried to modify \"%s\"",
                var_name);
        return;
    }
    Cvar_Set(var_name, value);
}

/*
============
Cvar_SetLatched
============
*/
void Cvar_SetLatched(const char *var_name, const char *value)
{
    Cvar_Set2(var_name, value, false);
}
/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue(const char *var_name, float value)
{
    char val[32];

    if (value == (int)value)
    {
        Com_sprintf(val, sizeof(val), "%i", (int)value);
    }
    else
    {
        Com_sprintf(val, sizeof(val), "%f", value);
    }
    Cvar_Set(var_name, val);
}

/*
============
Cvar_SetValueSafe
============
*/
void Cvar_SetValueSafe(const char *var_name, float value)
{
    char val[32];

    if (Q_isintegral(value))
        Com_sprintf(val, sizeof(val), "%i", (int)value);
    else
        Com_sprintf(val, sizeof(val), "%f", value);
    Cvar_SetSafe(var_name, val);
}

/*
============
Cvar_Reset
============
*/
void Cvar_Reset(const char *var_name)
{
    Cvar_Set2(var_name, nullptr, false);
}
/*
============
Cvar_ForceReset
============
*/
void Cvar_ForceReset(const char *var_name)
{
    Cvar_Set2(var_name, nullptr, true);
}
/*
============
Cvar_SetCheatState

Any testing variables will be reset to the safe values
============
*/
void Cvar_SetCheatState(void)
{
    // set all default vars to the safe value
    for (cvar_t *var = cvar_vars; var; var = var->next)
    {
        if (var->flags & CVAR_CHEAT)
        {
            // the CVAR_LATCHED|CVAR_CHEAT vars might escape the reset here
            // because of a different var->latchedString
            if (var->latchedString)
            {
                Z_Free(var->latchedString);
                var->latchedString = nullptr;
            }
            if (strcmp(var->resetString, var->string))
                Cvar_Set(var->name, var->resetString);
        }
    }
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
bool Cvar_Command(void)
{
    cvar_t *v = Cvar_FindVar(Cmd_Argv(0));
    if (!v)
    {
        return false;
    }

    // perform a variable print or set
    if (Cmd_Argc() == 1)
    {
        Cvar_Print(v);
        return true;
    }

    // set the value if forcing isn't required
    Cvar_Set2(v->name, Cmd_Args(), false);
    return true;
}

/*
============
Cvar_Print_f

Prints the contents of a cvar
(preferred over Cvar_Command where cvar names and commands conflict)
============
*/
void Cvar_Print_f(void)
{
    if (Cmd_Argc() != 2)
    {
        Com_Printf("usage: print <variable>\n");
        return;
    }

    const char *name = Cmd_Argv(1);
    cvar_t *cv = Cvar_FindVar(name);

    if (cv)
        Cvar_Print(cv);
    else
        Com_Printf("Cvar %s does not exist.\n", name);
}

/*
============
Cvar_Toggle_f

Toggles a cvar for easy single key binding, optionally through a list of
given values
============
*/
void Cvar_Toggle_f(void)
{
    int c = Cmd_Argc();
    if (c < 2)
    {
        Com_Printf("usage: toggle <variable> [value1, value2, ...]\n");
        return;
    }
    else if (c == 2)
    {
        Cvar_Set2(Cmd_Argv(1), va("%d", !Cvar_VariableValue(Cmd_Argv(1))), false);
        return;
    }
    else if (c == 3)
    {
        Com_Printf("toggle: nothing to toggle to\n");
        return;
    }

    const char *curval = Cvar_VariableString(Cmd_Argv(1));

    // don't bother checking the last arg for a match since the desired
    // behaviour is the same as no match (set to the first argument)
    for (int i = 2; i + 1 < c; i++)
    {
        if (strcmp(curval, Cmd_Argv(i)) == 0)
        {
            Cvar_Set2(Cmd_Argv(1), Cmd_Argv(i + 1), false);
            return;
        }
    }

    // fallback
    Cvar_Set2(Cmd_Argv(1), Cmd_Argv(2), false);
}

/*
============
Cvar_Set_f

Allows setting and defining of arbitrary cvars from console, even if they
weren't declared in C code.
============
*/
void Cvar_Set_f(void)
{
    int c = Cmd_Argc();
    const char *cmd = Cmd_Argv(0);

    if (c < 2)
    {
        Com_Printf("usage: %s <variable> <value>\n", cmd);
        return;
    }
    else if (c == 2)
    {
        Cvar_Print_f();
        return;
    }

    cvar_t *v = Cvar_Set2(Cmd_Argv(1), Cmd_ArgsFrom(2), false);
    if (!v)
    {
        return;
    }

    switch (cmd[3])
    {
        case 'a':
            if (!(v->flags & CVAR_ARCHIVE))
            {
                v->flags |= CVAR_ARCHIVE;
                cvar_modifiedFlags |= CVAR_ARCHIVE;
            }
            break;
        case 'u':
            if (!(v->flags & CVAR_USERINFO))
            {
                v->flags |= CVAR_USERINFO;
                cvar_modifiedFlags |= CVAR_USERINFO;
            }
            break;
        case 's':
            if (!(v->flags & CVAR_SERVERINFO))
            {
                v->flags |= CVAR_SERVERINFO;
                cvar_modifiedFlags |= CVAR_SERVERINFO;
            }
            break;
    }
}

/*
============
Cvar_Reset_f
============
*/
void Cvar_Reset_f(void)
{
    if (Cmd_Argc() != 2)
    {
        Com_Printf("usage: reset <variable>\n");
        return;
    }
    Cvar_Reset(Cmd_Argv(1));
}

/*
============
Cvar_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables(fileHandle_t f)
{
    cvar_t *var;
    char buffer[1024];

    for (var = cvar_vars; var; var = var->next)
    {
        if (!var->name)
            continue;

        if (var->flags & CVAR_ARCHIVE)
        {
            // write the latched value, even if it hasn't taken effect yet
            if (var->latchedString)
            {
                if (strlen(var->name) + strlen(var->latchedString) + 10 > sizeof(buffer))
                {
                    Com_Printf(S_COLOR_YELLOW
                        "WARNING: value of variable "
                        "\"%s\" too long to write to file\n",
                        var->name);
                    continue;
                }
                Com_sprintf(buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->latchedString);
            }
            else
            {
                if (strlen(var->name) + strlen(var->string) + 10 > sizeof(buffer))
                {
                    Com_Printf(S_COLOR_YELLOW
                        "WARNING: value of variable "
                        "\"%s\" too long to write to file\n",
                        var->name);
                    continue;
                }
                Com_sprintf(buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->string);
            }
            FS_Write(buffer, strlen(buffer), f);
        }
    }
}

/*
============
Cvar_List_f
============
*/
void Cvar_List_f(void)
{
    cvar_t *var;
    int i;
    const char *match;

    if (Cmd_Argc() > 1)
    {
        match = Cmd_Argv(1);
    }
    else
    {
        match = nullptr;
    }

    i = 0;
    for (var = cvar_vars; var; var = var->next, i++)
    {
        if (!var->name || (match && !Com_Filter(match, var->name, false)))
            continue;

        if (var->flags & CVAR_SERVERINFO)
        {
            Com_Printf("S");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_SYSTEMINFO)
        {
            Com_Printf("s");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_USERINFO)
        {
            Com_Printf("U");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_ROM)
        {
            Com_Printf("R");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_INIT)
        {
            Com_Printf("I");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_ARCHIVE)
        {
            Com_Printf("A");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_LATCH)
        {
            Com_Printf("L");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_CHEAT)
        {
            Com_Printf("C");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_USER_CREATED)
        {
            Com_Printf("?");
        }
        else
        {
            Com_Printf(" ");
        }

        Com_Printf(" %s \"%s\"\n", var->name, var->string);
    }

    Com_Printf("\n%i total cvars\n", i);
    Com_Printf("%i cvar indexes\n", cvar_numIndexes);
}

/*
============
Cvar_ListModified_f
============
*/
void Cvar_ListModified_f(void)
{
    cvar_t *var;
    int totalModified;
    char *value;
    const char *match;

    if (Cmd_Argc() > 1)
    {
        match = Cmd_Argv(1);
    }
    else
    {
        match = nullptr;
    }

    totalModified = 0;
    for (var = cvar_vars; var; var = var->next)
    {
        if (!var->name || !var->modificationCount)
            continue;

        value = var->latchedString ? var->latchedString : var->string;
        if (!strcmp(value, var->resetString))
            continue;

        totalModified++;

        if (match && !Com_Filter(match, var->name, false))
            continue;

        if (var->flags & CVAR_SERVERINFO)
        {
            Com_Printf("S");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_SYSTEMINFO)
        {
            Com_Printf("s");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_USERINFO)
        {
            Com_Printf("U");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_ROM)
        {
            Com_Printf("R");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_INIT)
        {
            Com_Printf("I");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_ARCHIVE)
        {
            Com_Printf("A");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_LATCH)
        {
            Com_Printf("L");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_CHEAT)
        {
            Com_Printf("C");
        }
        else
        {
            Com_Printf(" ");
        }
        if (var->flags & CVAR_USER_CREATED)
        {
            Com_Printf("?");
        }
        else
        {
            Com_Printf(" ");
        }

        Com_Printf(" %s \"%s\", default \"%s\"\n", var->name, value, var->resetString);
    }

    Com_Printf("\n%i total modified cvars\n", totalModified);
}

/*
============
Cvar_Unset

Unsets a cvar
============
*/

cvar_t *Cvar_Unset(cvar_t *cv)
{
    cvar_t *next = cv->next;

    // note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
    cvar_modifiedFlags |= cv->flags;

    if (cv->name)
        Z_Free(cv->name);
    if (cv->string)
        Z_Free(cv->string);
    if (cv->latchedString)
        Z_Free(cv->latchedString);
    if (cv->resetString)
        Z_Free(cv->resetString);
    if (cv->description)
        Z_Free(cv->description);

    if (cv->prev)
        cv->prev->next = cv->next;
    else
        cvar_vars = cv->next;
    if (cv->next)
        cv->next->prev = cv->prev;

    if (cv->hashPrev)
        cv->hashPrev->hashNext = cv->hashNext;
    else
        hashTable[cv->hashIndex] = cv->hashNext;
    if (cv->hashNext)
        cv->hashNext->hashPrev = cv->hashPrev;

    ::memset(cv, '\0', sizeof(*cv));

    return next;
}

/*
============
Cvar_Unset_f

Unsets a userdefined cvar
============
*/

void Cvar_Unset_f(void)
{
    cvar_t *cv;

    if (Cmd_Argc() != 2)
    {
        Com_Printf("Usage: %s <varname>\n", Cmd_Argv(0));
        return;
    }

    cv = Cvar_FindVar(Cmd_Argv(1));

    if (!cv)
        return;

    if (cv->flags & CVAR_USER_CREATED)
        Cvar_Unset(cv);
    else
        Com_Printf("Error: %s: Variable %s is not user created.\n", Cmd_Argv(0), cv->name);
}

/*
============
Cvar_Restart

Resets all cvars to their hardcoded values and removes userdefined variables
and variables added via the VMs if requested.
============
*/

void Cvar_Restart(bool unsetVM)
{
    cvar_t *curvar;

    curvar = cvar_vars;

    while (curvar)
    {
        if ((curvar->flags & CVAR_USER_CREATED) || (unsetVM && (curvar->flags & CVAR_VM_CREATED)))
        {
            // throw out any variables the user/vm created
            curvar = Cvar_Unset(curvar);
            continue;
        }

        if (!(curvar->flags & (CVAR_ROM | CVAR_INIT | CVAR_NORESTART)))
        {
            // Just reset the rest to their default values.
            Cvar_Set2(curvar->name, curvar->resetString, false);
        }

        curvar = curvar->next;
    }
}

/*
============
Cvar_Restart_f

Resets all cvars to their hardcoded values
============
*/
void Cvar_Restart_f(void)
{
    Cvar_Restart(false);
}

/*
=====================
Cvar_InfoString
=====================
*/
char *Cvar_InfoString(int bit)
{
    static char	info[MAX_INFO_STRING];
    cvar_t	*var;

    info[0] = 0;

    for(var = cvar_vars; var; var = var->next)
    {
        if(var->name && (var->flags & bit)) {
            if(var->flags & CVAR_REMOVE_UNUSED_COLOR_STRINGS) {
                char cleaned_string[MAX_CVAR_VALUE_STRING];

                Q_RemoveUnusedColorStrings(var->string, cleaned_string, MAX_CVAR_VALUE_STRING);
                if(Q_stricmp(cleaned_string, var->string)) {
                    Cvar_Set(var->name, cleaned_string);
                }
            }
            Info_SetValueForKey (info, var->name, var->string);
        }
    }

    return info;
}

/*
=====================
Cvar_InfoString_Big

  handles large info strings ( CS_SYSTEMINFO )
=====================
*/
char *Cvar_InfoString_Big(int bit)
{
    static char info[BIG_INFO_STRING];
    cvar_t *var;

    info[0] = 0;

    for (var = cvar_vars; var; var = var->next)
    {
        if (var->name && (var->flags & bit))
            Info_SetValueForKey_Big(info, var->name, var->string);
    }
    return info;
}

/*
=====================
Cvar_InfoStringBuffer
=====================
*/
void Cvar_InfoStringBuffer(int bit, char *buff, int buffsize)
{
    Q_strncpyz(buff, Cvar_InfoString(bit), buffsize);
}
/*
=====================
Cvar_CheckRange
=====================
*/
void Cvar_CheckRange(cvar_t *var, float min, float max, bool integral)
{
    var->validate = true;
    var->min = min;
    var->max = max;
    var->integral = integral;

    // Force an initial range check
    Cvar_Set(var->name, var->string);
}

/*
=====================
Cvar_SetDescription
=====================
*/
void Cvar_SetDescription(cvar_t *var, const char *var_description)
{
    if (var_description && var_description[0] != '\0')
    {
        if (var->description != nullptr)
        {
            Z_Free(var->description);
        }
        var->description = CopyString(var_description);
    }
}

/*
=====================
Cvar_Register

basically a slightly modified Cvar_Get for the interpreted modules
=====================
*/
void Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags)
{
    cvar_t *cv;

    // There is code in Cvar_Get to prevent CVAR_ROM cvars being changed by the
    // user. In other words CVAR_ARCHIVE and CVAR_ROM are mutually exclusive
    // flags. Unfortunately some historical game code (including single player
    // baseq3) sets both flags. We unset CVAR_ROM for such cvars.
    if ((flags & (CVAR_ARCHIVE | CVAR_ROM)) == (CVAR_ARCHIVE | CVAR_ROM))
    {
        Com_DPrintf(S_COLOR_YELLOW
            "WARNING: Unsetting CVAR_ROM cvar '%s', "
            "since it is also CVAR_ARCHIVE\n",
            varName);
        flags &= ~CVAR_ROM;
    }

    cv = Cvar_Get(varName, defaultValue, flags | CVAR_VM_CREATED);

    if (!vmCvar)
        return;

    vmCvar->handle = cv - cvar_indexes;
    vmCvar->modificationCount = -1;
    Cvar_Update(vmCvar);
}

/*
=====================
Cvar_Update

updates an interpreted modules' version of a cvar
=====================
*/
void Cvar_Update(vmCvar_t *vmCvar)
{
    cvar_t *cv = nullptr;
    assert(vmCvar);

    if (vmCvar->handle >= cvar_numIndexes)
    {
        Com_Error(ERR_DROP, "Cvar_Update: handle out of range");
    }

    cv = cvar_indexes + vmCvar->handle;

    if (cv->modificationCount == vmCvar->modificationCount)
    {
        return;
    }
    if (!cv->string)
    {
        return;  // variable might have been cleared by a cvar_restart
    }
    vmCvar->modificationCount = cv->modificationCount;
    if (strlen(cv->string) + 1 > MAX_CVAR_VALUE_STRING)
        Com_Error(ERR_DROP, "Cvar_Update: src %s length %u exceeds MAX_CVAR_VALUE_STRING", cv->string,
            (unsigned int)strlen(cv->string));
    Q_strncpyz(vmCvar->string, cv->string, MAX_CVAR_VALUE_STRING);

    vmCvar->value = cv->value;
    vmCvar->integer = cv->integer;
}

/*
==================
Cvar_CompleteCvarName
==================
*/
void Cvar_CompleteCvarName(char *args, int argNum)
{
    if (argNum == 2)
    {
        // Skip "<cmd> "
        char *p = Com_SkipTokens(args, 1, " ");

        if (p > args)
            Field_CompleteCommand(p, false, true);
    }
}

/*
============
Cvar_Init

Reads in all archived cvars
============
*/
void Cvar_Init(void)
{
    ::memset(cvar_indexes, '\0', sizeof(cvar_indexes));
    ::memset(hashTable, '\0', sizeof(hashTable));

    cvar_cheats = Cvar_Get("sv_cheats", "1", CVAR_ROM | CVAR_SYSTEMINFO);

    Cmd_AddCommand("print", Cvar_Print_f);
    Cmd_AddCommand("toggle", Cvar_Toggle_f);
    Cmd_SetCommandCompletionFunc("toggle", Cvar_CompleteCvarName);
    Cmd_AddCommand("set", Cvar_Set_f);
    Cmd_SetCommandCompletionFunc("set", Cvar_CompleteCvarName);
    Cmd_AddCommand("sets", Cvar_Set_f);
    Cmd_SetCommandCompletionFunc("sets", Cvar_CompleteCvarName);
    Cmd_AddCommand("setu", Cvar_Set_f);
    Cmd_SetCommandCompletionFunc("setu", Cvar_CompleteCvarName);
    Cmd_AddCommand("seta", Cvar_Set_f);
    Cmd_SetCommandCompletionFunc("seta", Cvar_CompleteCvarName);
    Cmd_AddCommand("reset", Cvar_Reset_f);
    Cmd_SetCommandCompletionFunc("reset", Cvar_CompleteCvarName);
    Cmd_AddCommand("unset", Cvar_Unset_f);
    Cmd_SetCommandCompletionFunc("unset", Cvar_CompleteCvarName);

    Cmd_AddCommand("cvarlist", Cvar_List_f);
    Cmd_AddCommand("cvar_modified", Cvar_ListModified_f);
    Cmd_AddCommand("cvar_restart", Cvar_Restart_f);
}
