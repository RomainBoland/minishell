/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   environment.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: evan-dro <evan-dro@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/19 11:49:55 by rboland           #+#    #+#             */
/*   Updated: 2025/04/17 16:52:19 by evan-dro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

// Create a new environment variable node
t_env *create_env_node(char *key, char *value)
{
    t_env *new_node;
    
    new_node = malloc(sizeof(t_env));
    if (!new_node)
        return NULL;
    
    new_node->key = ft_strdup(key);
    new_node->value = value ? ft_strdup(value) : NULL;
    new_node->next = NULL;
    
    return new_node;
}

int get_shlvl_from_envp(char **envp)
{
    for (int i = 0; envp[i]; i++)
    {
        if (ft_strncmp(envp[i], "SHLVL=", 6) == 0)
            return ft_atoi(envp[i] + 6);
    }
    return 0;
}



// Initialize environment variables from envp
t_env *init_env(char **envp)
{
    t_env *env_list = NULL;
    t_env *current = NULL;
    t_env *new_node;
    int i;
    char *equals_sign;
    int is_in_minishell = 0;

    
    if (!envp)
        return NULL;

    for (i = 0; envp[i]; i++)
    {
        // Vérifie si on est déjà dans minishell
        if (ft_strncmp(envp[i], "is_in_minishell=true", 21) == 0)
            is_in_minishell = 1;

        // Ignore SHLVL, on le gère nous-mêmes
        if (ft_strncmp(envp[i], "SHLVL=", 6) == 0)
            continue;

        equals_sign = ft_strchr(envp[i], '=');
        if (equals_sign)
        {
            *equals_sign = '\0';
            new_node = create_env_node(envp[i], equals_sign + 1);
            *equals_sign = '=';

            if (!new_node)
                continue;

            if (!env_list)
                env_list = new_node;
            else
                current->next = new_node;

            current = new_node;
        }
    }

    // Gère SHLVL
    int shlvl = get_shlvl_from_envp(envp); // Lire la vraie valeur
    if (is_in_minishell)
        shlvl++;
    if (shlvl < 0)
        shlvl = 0;
    char shlvl_str[12];
    sprintf(shlvl_str, "%d", shlvl);
    new_node = create_env_node("SHLVL", shlvl_str);
    if (!new_node)
        return env_list;
    if (current)
        current->next = new_node;
    else
        env_list = new_node;
    current = new_node;

    // Ajoute is_in_minishell=true
    new_node = create_env_node("is_in_minishell", "true");
    if (!new_node)
        return env_list;
    current->next = new_node;

    return env_list;
}


// Free all environment variables
void free_env(t_env *env)
{
    t_env *tmp;
    
    while (env)
    {
        tmp = env;
        env = env->next;
        
        free(tmp->key);
        free(tmp->value);
        free(tmp);
    }
}

// Get the value of an environment variable 
char *get_env_value(t_env *env, char *key)
{
    while (env)
    {
        if (ft_strncmp(env->key, key, ft_strlen(key) + 1) == 0)
            return env->value;
        env = env->next;
    }
    
    return NULL;
}

// Set an environment variable's value (or create if it doesn't exist)
void set_env_value(t_env *env, char *key, char *value)
{
    t_env *prev = NULL;
    t_env *new_node;
    
    // Look for existing variable
    while (env)
    {
        if (ft_strncmp(env->key, key, ft_strlen(key) + 1) == 0)
        {
            // Update existing variable
            free(env->value);
            env->value = value ? ft_strdup(value) : NULL;
            return;
        }
        prev = env;
        env = env->next;
    }
    
    // Variable doesn't exist, create it
    new_node = create_env_node(key, value);
    if (new_node && prev)
        prev->next = new_node;
}

// Remove an environment variable
void remove_env_var(t_env *env, char *key)
{
    t_env *prev = NULL;
    t_env *current = env;
    
    while (current)
    {
        if (ft_strncmp(current->key, key, ft_strlen(key) + 1) == 0)
        {
            if (prev)
                prev->next = current->next;
            
            free(current->key);
            free(current->value);
            free(current);
            
            return;
        }
        
        prev = current;
        current = current->next;
    }
}

// Convert environment linked list to array for execve
char **env_to_array(t_env *env)
{
    int count = 0;
    t_env *current = env;
    char **env_array;
    int i;
    
    // Count environment variables
    while (current)
    {
        count++;
        current = current->next;
    }
    
    // Allocate array
    env_array = malloc(sizeof(char *) * (count + 1));
    if (!env_array)
        return NULL;
    
    // Fill array
    current = env;
    i = 0;
    while (current)
    {
        if (current->value == NULL)
        {
            current = current->next;
            continue;
        }
        env_array[i] = malloc(ft_strlen(current->key) + ft_strlen(current->value) + 2);
        if (!env_array[i])
        {
            // Clean up on error
            while (--i >= 0)
                free(env_array[i]);
            free(env_array);
            return NULL;
        }
        
        // Fill buffer with "key=value"
        ft_strlcpy(env_array[i], current->key, ft_strlen(current->key) + 1);
        ft_strlcat(env_array[i], "=", ft_strlen(current->key) + 2);
        ft_strlcat(env_array[i], current->value, ft_strlen(current->key) + ft_strlen(current->value) + 2);
        
        current = current->next;
        i++;
    }
    
    env_array[i] = NULL;
    return env_array;
}

char **env_to_array_export(t_env *env)
{
    int count = 0;
    int i = 0;
    t_env *current = env;
    char **env_array;
    
    // Compter le nombre de variables
    while (current)
    {
        if (!(ft_strcmp(current->key, "_") == 0 && current->value != NULL))
            count++;
        current = current->next;
    }
    
    env_array = malloc(sizeof(char *) * (count + 1));
    if (!env_array)
        return NULL;
    
    current = env;
    while (current)
    {

        if (ft_strcmp(current->key, "_") == 0 && current->value != NULL)
        {
            current = current->next;
            continue;
        }

        int len = ft_strlen(current->key);
        if (current->value)
            len += ft_strlen(current->value) + 4; // = + "" + \0
        
        env_array[i] = malloc(len + 1);
        if (!env_array[i])
        {
            while (--i >= 0)
                free(env_array[i]);
            free(env_array);
            return NULL;
        }
        
        if (current->value == NULL)
        {
            // Variable déclarée sans valeur explicite
            ft_strlcpy(env_array[i], current->key, len + 1);
        }
        else
        {
            // Variable avec valeur (même vide)
            ft_strlcpy(env_array[i], current->key, len + 1);
            ft_strlcat(env_array[i], "=\"", len + 1);
            ft_strlcat(env_array[i], current->value, len + 1);
            ft_strlcat(env_array[i], "\"", len + 1);
        }
        
        i++;
        current = current->next;
    }
    
    env_array[i] = NULL;
    return (env_array);
}

// Check if an environment variable exists
int has_env_key(t_env *env, char *key)
{
    while (env)
    {
        if (ft_strncmp(env->key, key, ft_strlen(key) + 1) == 0)
            return 1;
        env = env->next;
    }
    return 0;
}

// Free environment array
void free_env_array(char **env_array)
{
    int i;
    
    if (!env_array)
        return;
    
    for (i = 0; env_array[i]; i++)
        free(env_array[i]);
    
    free(env_array);
}
