#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <time.h>
#include <ctype.h>
#include "entraineur.h"

/******************* Helper Function for Password Generation *******************/

void generer_mot_passe(char *mot_passe, int longueur) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int charset_size = strlen(charset);
    
    // Initialize random seed only once
    static int seed_initialized = 0;
    if (!seed_initialized) {
        srand(time(NULL));
        seed_initialized = 1;
    }
    
    for (int i = 0; i < longueur; i++) {
        mot_passe[i] = charset[rand() % charset_size];
    }
    mot_passe[longueur] = '\0';
}

/******************* Helper Function for Auto-Increment *******************/

int get_next_id() {
    entraineur e;
    char date[12];
    int max_id = 0;
    FILE *f = fopen("entraineur.txt", "r");
    
    if (f) {
        while (fscanf(f, "%d %199s %299s %299s %9s %299s %11s %1s %199s\n",
                      &e.id_e, e.cin_e, e.nom_e, e.prenom_e, e.sexe_e, e.specialite_e,
                      date, e.entrainement_prive, e.mot_passe_e) != EOF) {
            if (e.id_e > max_id) {   
                max_id = e.id_e;
            }
        }
        fclose(f);
    }
    return max_id + 1;
}

/******************* CRUD Functions *******************/

void ajouter_entraineur(entraineur e) {
    FILE *f = fopen("entraineur.txt", "a");
    if (f) {
        // Auto-increment the ID
        e.id_e = get_next_id();
        
        // Auto-generate password (8 characters)
        generer_mot_passe(e.mot_passe_e, 8);
        
        fprintf(f, "%d %s %s %s %s %s %02d/%02d/%04d %s %s\n",
                e.id_e, e.cin_e, e.nom_e, e.prenom_e, e.sexe_e, e.specialite_e,
                e.date_nai_e.j, e.date_nai_e.m, e.date_nai_e.a,
                e.entrainement_prive, e.mot_passe_e);
        fflush(f);
        fclose(f);
    }
}

int entraineur_existe(const char *cin) {
    entraineur e;
    char date[12];
    FILE *f = fopen("entraineur.txt", "r");
    if (!f) return 0;

    while (fscanf(f, "%d %199s %299s %299s %9s %299s %11s %1s %199s\n",
                  &e.id_e, e.cin_e, e.nom_e, e.prenom_e, e.sexe_e, e.specialite_e,
                  date, e.entrainement_prive, e.mot_passe_e) != EOF) {
        sscanf(date, "%d/%d/%d", &e.date_nai_e.j, &e.date_nai_e.m, &e.date_nai_e.a);
        if (strcmp(e.cin_e, cin) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void supprimer_entraineur(const char *cin) {
    entraineur e;
    char date[12];
    FILE *f = fopen("entraineur.txt", "r");
    FILE *temp = fopen("temp.txt", "w");

    if (f && temp) {
        while (fscanf(f, "%d %199s %299s %299s %9s %299s %11s %1s %199s\n",
                      &e.id_e, e.cin_e, e.nom_e, e.prenom_e, e.sexe_e, e.specialite_e,
                      date, e.entrainement_prive, e.mot_passe_e) != EOF) {
            sscanf(date, "%d/%d/%d", &e.date_nai_e.j, &e.date_nai_e.m, &e.date_nai_e.a);
            if (strcmp(e.cin_e, cin) != 0) {
                fprintf(temp, "%d %s %s %s %s %s %02d/%02d/%04d %s %s\n",
                        e.id_e, e.cin_e, e.nom_e, e.prenom_e, e.sexe_e, e.specialite_e,
                        e.date_nai_e.j, e.date_nai_e.m, e.date_nai_e.a,
                        e.entrainement_prive, e.mot_passe_e);
            }
        }
    }
    if (f) fclose(f);
    if (temp) fclose(temp);
    remove("entraineur.txt");
    rename("temp.txt", "entraineur.txt");
}

void modifier_entraineur(entraineur e) {
    entraineur temp_e;
    char date[12];
    FILE *f = fopen("entraineur.txt", "r");
    FILE *temp = fopen("temp.txt", "w");

    if (!f || !temp) {
        printf("Erreur : Impossible d'ouvrir le fichier pour modification.\n");
        if (f) fclose(f);
        if (temp) fclose(temp);
        return;
    }

    while (fscanf(f, "%d %199s %299s %299s %9s %299s %11s %1s %199s\n",
                  &temp_e.id_e, temp_e.cin_e, temp_e.nom_e, temp_e.prenom_e, temp_e.sexe_e, temp_e.specialite_e,
                  date, temp_e.entrainement_prive, temp_e.mot_passe_e) != EOF) {
        sscanf(date, "%d/%d/%d", &temp_e.date_nai_e.j, &temp_e.date_nai_e.m, &temp_e.date_nai_e.a);

        if (strcmp(temp_e.cin_e, e.cin_e) == 0) {
            // Replace with updated entraineur (keep the original ID and password)
            fprintf(temp, "%d %s %s %s %s %s %02d/%02d/%04d %s %s\n",
                    temp_e.id_e, e.cin_e, e.nom_e, e.prenom_e, e.sexe_e, e.specialite_e,
                    e.date_nai_e.j, e.date_nai_e.m, e.date_nai_e.a,
                    e.entrainement_prive, temp_e.mot_passe_e); // Keep original password
        } else {
            // Keep the original entraineur
            fprintf(temp, "%d %s %s %s %s %s %02d/%02d/%04d %s %s\n",
                    temp_e.id_e, temp_e.cin_e, temp_e.nom_e, temp_e.prenom_e, temp_e.sexe_e, temp_e.specialite_e,
                    temp_e.date_nai_e.j, temp_e.date_nai_e.m, temp_e.date_nai_e.a,
                    temp_e.entrainement_prive, temp_e.mot_passe_e);
        }
    }

    fclose(f);
    fclose(temp);

    remove("entraineur.txt");
    rename("temp.txt", "entraineur.txt");
}

entraineur trouver_entraineur(const char *cin) {
    entraineur e;
    char date[12];
    FILE *f = fopen("entraineur.txt", "r");

    if (f) {
        while (fscanf(f, "%d %199s %299s %299s %9s %299s %11s %1s %199s\n",
                      &e.id_e, e.cin_e, e.nom_e, e.prenom_e, e.sexe_e, e.specialite_e,
                      date, e.entrainement_prive, e.mot_passe_e) != EOF) {
            sscanf(date, "%d/%d/%d", &e.date_nai_e.j, &e.date_nai_e.m, &e.date_nai_e.a);
            if (strcmp(e.cin_e, cin) == 0) {
                fclose(f);
                return e;
            }
        }
        fclose(f);
    }
    entraineur empty = {0, "", "", "", "", "", {0, 0, 0}, "", ""};
    return empty;
}

/******************* Display Function *******************/

enum {
    ID_E,
    CIN,
    NOM,
    PRENOM,
    SEXE,
    SPECIALITE,
    DATE_NAISSANCE,
    ENTRAINEMENT_PRIVE,
    COLUMNS_E
};

void afficher_entraineurs(GtkWidget *list) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeIter iter;
    GtkListStore *store;

    int id_e;
    char cin_e[200], nom_e[300], prenom_e[300], sexe_e[10], specialite_e[300];
    char entrainement_prive[2], mot_passe_e[200];
    int jour, mois, annee;
    char date[12];

    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));

    if (store == NULL) {
        // Create columns for the TreeView
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", ID_E, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("CIN", renderer, "text", CIN, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", NOM, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Prenom", renderer, "text", PRENOM, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Sexe", renderer, "text", SEXE, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Specialite", renderer, "text", SPECIALITE, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Date de Naissance", renderer, "text", DATE_NAISSANCE, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Entrainement Privé", renderer, "text", ENTRAINEMENT_PRIVE, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        // Create ListStore with appropriate types
        store = gtk_list_store_new(COLUMNS_E,
                                   G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        
        // Set the model ONCE during initialization
        gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));
        g_object_unref(store);
    }

    // Clear the store (now it's safe because it's already attached)
    gtk_list_store_clear(store);

    FILE *f = fopen("entraineur.txt", "r");
    if (!f) {
        printf("Erreur : impossible d'ouvrir le fichier entraineur.txt.\n");
        return;
    }

    // Read data from file and populate the ListStore
    while (fscanf(f, "%d %199s %299s %299s %9s %299s %d/%d/%d %1s %199s\n",
                  &id_e, cin_e, nom_e, prenom_e, sexe_e, specialite_e,
                  &jour, &mois, &annee, entrainement_prive, mot_passe_e) != EOF) {
        snprintf(date, sizeof(date), "%02d/%02d/%04d", jour, mois, annee);

        // Convert entrainement_prive to readable text
        char prive_text[10];
        strcpy(prive_text, (strcmp(entrainement_prive, "1") == 0) ? "Oui" : "Non");

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           ID_E, id_e,
                           CIN, cin_e,
                           NOM, nom_e,
                           PRENOM, prenom_e,
                           SEXE, sexe_e,
                           SPECIALITE, specialite_e,
                           DATE_NAISSANCE, date,
                           ENTRAINEMENT_PRIVE, prive_text,
                           -1);
    }

    fclose(f);
}

/******************* Additional Functions *******************/

void supprimer_entraineur_par_mdp(const char *mot_passe) {
    entraineur e;
    char date[12];
    FILE *f = fopen("entraineur.txt", "r");
    FILE *temp = fopen("temp.txt", "w");

    if (f && temp) {
        while (fscanf(f, "%d %199s %299s %299s %9s %299s %11s %1s %199s\n",
                      &e.id_e, e.cin_e, e.nom_e, e.prenom_e, e.sexe_e, e.specialite_e,
                      date, e.entrainement_prive, e.mot_passe_e) != EOF) {
            sscanf(date, "%d/%d/%d", &e.date_nai_e.j, &e.date_nai_e.m, &e.date_nai_e.a);
            
            // Keep records that don't match the password
            if (strcmp(e.mot_passe_e, mot_passe) != 0) {
                fprintf(temp, "%d %s %s %s %s %s %02d/%02d/%04d %s %s\n",
                        e.id_e, e.cin_e, e.nom_e, e.prenom_e, e.sexe_e, e.specialite_e,
                        e.date_nai_e.j, e.date_nai_e.m, e.date_nai_e.a,
                        e.entrainement_prive, e.mot_passe_e);
            }
        }
    }
    if (f) fclose(f);
    if (temp) fclose(temp);

    remove("entraineur.txt");
    rename("temp.txt", "entraineur.txt");
}

/******************* Search Function *******************/

int rechercher_entraineurs(const char *critere, GtkWidget *treeview) {
    entraineur e;
    char date[12];
    char critere_lower[600];
    int found = 0;
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeIter iter;
    GtkListStore *store;
    
    if (!treeview) return 0;
    
    // Convert search criteria to lowercase
    strcpy(critere_lower, critere);
    for (int i = 0; critere_lower[i]; i++) {
        critere_lower[i] = tolower(critere_lower[i]);
    }
    
    // Get or create store
    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));
    
    if (store == NULL) {
        // Create columns for the TreeView
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", ID_E, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("CIN", renderer, "text", CIN, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", NOM, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Prenom", renderer, "text", PRENOM, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Sexe", renderer, "text", SEXE, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Specialite", renderer, "text", SPECIALITE, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Date de Naissance", renderer, "text", DATE_NAISSANCE, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Entrainement Privé", renderer, "text", ENTRAINEMENT_PRIVE, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        store = gtk_list_store_new(COLUMNS_E,
                                   G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        
        // Set the model ONCE during initialization
        gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
        g_object_unref(store);
    }
    
    // Clear the store (now it's safe)
    gtk_list_store_clear(store);
    
    // Open file
    FILE *f = fopen("entraineur.txt", "r");
    if (!f) {
        printf("Erreur : impossible d'ouvrir le fichier entraineur.txt.\n");
        return 0;
    }
    
    // Read and search
    while (fscanf(f, "%d %199s %299s %299s %9s %299s %d/%d/%d %1s %199s\n",
                  &e.id_e, e.cin_e, e.nom_e, e.prenom_e, e.sexe_e, e.specialite_e,
                  &e.date_nai_e.j, &e.date_nai_e.m, &e.date_nai_e.a, 
                  e.entrainement_prive, e.mot_passe_e) != EOF) {
        
        // Convert all fields to lowercase for comparison
        char cin_lower[200], nom_lower[300], prenom_lower[300], sexe_lower[10], spec_lower[300];
        strcpy(cin_lower, e.cin_e);
        strcpy(nom_lower, e.nom_e);
        strcpy(prenom_lower, e.prenom_e);
        strcpy(sexe_lower, e.sexe_e);
        strcpy(spec_lower, e.specialite_e);
        
        for (int i = 0; cin_lower[i]; i++) cin_lower[i] = tolower(cin_lower[i]);
        for (int i = 0; nom_lower[i]; i++) nom_lower[i] = tolower(nom_lower[i]);
        for (int i = 0; prenom_lower[i]; i++) prenom_lower[i] = tolower(prenom_lower[i]);
        for (int i = 0; sexe_lower[i]; i++) sexe_lower[i] = tolower(sexe_lower[i]);
        for (int i = 0; spec_lower[i]; i++) spec_lower[i] = tolower(spec_lower[i]);
        
        // Check if criteria matches any field (case-insensitive)
        if (strstr(cin_lower, critere_lower) != NULL ||
            strstr(nom_lower, critere_lower) != NULL ||
            strstr(prenom_lower, critere_lower) != NULL ||
            strstr(sexe_lower, critere_lower) != NULL ||
            strstr(spec_lower, critere_lower) != NULL) {
            
            found++;
            
            // Format date
            char date_str[12];
            snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d", 
                    e.date_nai_e.j, e.date_nai_e.m, e.date_nai_e.a);
            
            // Convert entrainement_prive to readable text
            char prive_text[10];
            strcpy(prive_text, (strcmp(e.entrainement_prive, "1") == 0) ? "Oui" : "Non");
            
            // Add to store
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                              ID_E, e.id_e,
                              CIN, e.cin_e,
                              NOM, e.nom_e,
                              PRENOM, e.prenom_e,
                              SEXE, e.sexe_e,
                              SPECIALITE, e.specialite_e,
                              DATE_NAISSANCE, date_str,
                              ENTRAINEMENT_PRIVE, prive_text,
                              -1);
        }
    }
    
    fclose(f);
    
    return found;
}
