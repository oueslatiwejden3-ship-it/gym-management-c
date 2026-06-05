#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "treeview.h"
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "centres.h"
// Global path for private trainers file
extern char ENTRAINEUR_PRIVE_FILE_PATH[256];
 extern char COURS_FILE_PATH[256];

// Helper function to get niveau name
static const char* get_niveau_nom(int niveau) {
    switch(niveau) {
        case 0: return "Debutant";
        case 1: return "Intermediaire";
        case 2: return "Avance";
        default: return "Inconnu";
    }
}

// Function to read a specific cours from file
static Cours lire_cours(const char *filename, const char *id_cours) {
    Cours c;
    FILE *f = fopen(filename, "r");
    strcpy(c.id_cours, "-1");
    
    if (f != NULL) {
        while (fscanf(f, "%s %s %s %d %d %d %d %d %d %d %s %u", 
                     c.id_cours, c.nom_cours, c.type_activite, 
                     &c.jour, &c.mois, &c.annee, 
                     &c.heure_debut, &c.heure_fin, 
                     &c.niveau, &c.places_max, c.centre, &c.jours_mask) == 12) {
            if (strcmp(c.id_cours, id_cours) == 0) {
                fclose(f);
                return c;
            }
        }
        fclose(f);
        strcpy(c.id_cours, "-1");
    }
    return c;
}

int lire_tous_les_cours(const char *filename, Cours *tab, int max) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    
    int count = 0;
    while (count < max && fscanf(f, "%99s %99s %99s %d %d %d %d %d %d %d %99s %u\n",
                                  tab[count].id_cours,
                                  tab[count].nom_cours,
                                  tab[count].type_activite,
                                  &tab[count].jour,
                                  &tab[count].mois,
                                  &tab[count].annee,
                                  &tab[count].heure_debut,
                                  &tab[count].heure_fin,
                                  &tab[count].niveau,
                                  &tab[count].places_max,
                                  tab[count].centre,
                                  &tab[count].jours_mask) == 12) {  // 12 champs maintenant
        count++;
    }
    
    fclose(f);
    return count;
}

// Function to display courses for a trainer
void refresh_trainer_courses_treeview(GtkWidget *liste, const char *id_entraineur_param) {
    GtkListStore *store;
    GtkTreeModel *model;
    GtkTreeIter iter;
    FILE *f_inscriptions;
    char id_entr_lu[MAX_STR], id_cours[MAX_STR];
    Cours c;
    char date_str[50], horaire_str[50];
    
    if (liste == NULL) return;
    
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(liste));
    
    if (model == NULL) {
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        
        store = gtk_list_store_new(8, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, 
                                   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, 
                                   G_TYPE_INT, G_TYPE_STRING);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", 0, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(liste), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 1, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(liste), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 2, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(liste), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", 3, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(liste), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Horaire", renderer, "text", 4, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(liste), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Niveau", renderer, "text", 5, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(liste), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Places", renderer, "text", 6, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(liste), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Centre", renderer, "text", 7, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(liste), column);
        
        gtk_tree_view_set_model(GTK_TREE_VIEW(liste), GTK_TREE_MODEL(store));
        g_object_unref(store);
    } else {
        store = GTK_LIST_STORE(model);
        gtk_list_store_clear(store);
    }
    
    f_inscriptions = fopen(INSCRIPTION_ENTRAINEUR_FILE_PATH, "r");
    if (f_inscriptions != NULL) {
        while (fscanf(f_inscriptions, "%s %s", id_entr_lu, id_cours) == 2) {
            if (strcmp(id_entr_lu, id_entraineur_param) == 0) {
                c = lire_cours(COURS_FILE_PATH, id_cours);
                
                if (strcmp(c.id_cours, "-1") != 0) {
                    sprintf(date_str, "%02d/%02d/%04d", c.jour, c.mois, c.annee);
                    sprintf(horaire_str, "%02d:00-%02d:00", c.heure_debut, c.heure_fin);
                    
                    gtk_list_store_append(store, &iter);
                    gtk_list_store_set(store, &iter, 
                                      0, c.id_cours, 
                                      1, c.nom_cours, 
                                      2, c.type_activite, 
                                      3, date_str, 
                                      4, horaire_str, 
                                      5, get_niveau_nom(c.niveau), 
                                      6, c.places_max, 
                                      7, c.centre, 
                                      -1);
                }
            }
        }
        fclose(f_inscriptions);
    }
}

// Function to display all equipment
void refresh_equipment_treeview(GtkWidget *liste) {
    GtkListStore *store;
    GtkTreeModel *model;
    GtkTreeIter iter;
    Equipement e;
    FILE *f;
    
    if (liste == NULL) return;
    
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(liste));
    
    if (model == NULL) {
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        
        store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", 0, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(liste), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 1, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(liste), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 2, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(liste), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Quantité", renderer, "text", 3, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(liste), column);
        
        gtk_tree_view_set_model(GTK_TREE_VIEW(liste), GTK_TREE_MODEL(store));
        g_object_unref(store);
    } else {
        store = GTK_LIST_STORE(model);
        gtk_list_store_clear(store);
    }
    
    f = fopen(EQUIPEMENT_FILE_PATH, "r");
    if (f != NULL) {
        store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(liste)));
        while (fscanf(f, "%s %s %s %d", e.id_equipement, e.nom_equipement, e.type, &e.nombre) != EOF) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 
                              0, e.id_equipement, 
                              1, e.nom_equipement, 
                              2, e.type, 
                              3, e.nombre, 
                              -1);
        }
        fclose(f);
    }
}

// Function to load equipment types into combobox
void refresh_equipment_types_combobox(GtkWidget *combobox, int ajouter_aucune) {
    FILE *f;
    Equipement e;
    GtkTreeModel *model;
    char types[100][MAX_STR];
    int nb_types = 0;
    int i, j, existe;
    
    if (combobox == NULL) return;
    
    model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
    if (model != NULL) {
        gtk_list_store_clear(GTK_LIST_STORE(model));
    }
    
    if (ajouter_aucune) {
        gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Aucune");
    }
    
    f = fopen(EQUIPEMENT_FILE_PATH, "r");
    if (f != NULL) {
        while (fscanf(f, "%s %s %s %d", e.id_equipement, e.nom_equipement, e.type, &e.nombre) != EOF) {
            existe = 0;
            for (j = 0; j < nb_types; j++) {
                if (strcmp(types[j], e.type) == 0) {
                    existe = 1;
                    break;
                }
            }
            if (!existe && nb_types < 100) {
                strcpy(types[nb_types], e.type);
                nb_types++;
            }
        }
        fclose(f);
        
        for (i = 0; i < nb_types; i++) {
            gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), types[i]);
        }
    }
    
    gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
}

// Function to load activity types into combobox
void refresh_activity_types_combobox(GtkWidget *combobox) {
    FILE *f;
    Cours c;
    GtkTreeModel *model;
    char types[100][MAX_STR];
    int nb_types = 0;
    int i, j, existe;
    
    if (combobox == NULL) return;
    
    model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
    if (model != NULL) {
        gtk_list_store_clear(GTK_LIST_STORE(model));
    }
    
    gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Aucune");
    
    f = fopen(COURS_FILE_PATH, "r");
    if (f != NULL) {
        while (fscanf(f, "%s %s %s %d %d %d %d %d %d %d %s %u", 
                     c.id_cours, c.nom_cours, c.type_activite, 
                     &c.jour, &c.mois, &c.annee, 
                     &c.heure_debut, &c.heure_fin, 
                     &c.niveau, &c.places_max, c.centre, &c.jours_mask) == 12) {
            existe = 0;
            for (j = 0; j < nb_types; j++) {
                if (strcmp(types[j], c.type_activite) == 0) {
                    existe = 1;
                    break;
                }
            }
            if (!existe && nb_types < 100) {
                strcpy(types[nb_types], c.type_activite);
                nb_types++;
            }
        }
        fclose(f);
        
        for (i = 0; i < nb_types; i++) {
            gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), types[i]);
        }
    }
    
    gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
}

// Function to refresh admin courses treeview (shows all courses)
void refresh_admin_courses_treeview(GtkWidget *treeview) {
    if (!treeview) return;
    
    Cours tab[MAX_COURS];
    int n = lire_tous_les_cours(COURS_FILE_PATH, tab, MAX_COURS);
    
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    GtkListStore *store;
    
    if (model == NULL) {
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        
        store = gtk_list_store_new(8, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_INT, G_TYPE_STRING);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", 0, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 1, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 2, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", 3, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Horaire", renderer, "text", 4, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Niveau", renderer, "text", 5, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Places", renderer, "text", 6, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Centre", renderer, "text", 7, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
        g_object_unref(store);
    } else {
        store = GTK_LIST_STORE(model);
        gtk_list_store_clear(store);
    }
    
    GtkTreeIter iter;
    for (int i = 0; i < n; i++) {
        char date_str[50], horaire_str[50];
        sprintf(date_str, "%02d/%02d/%04d", tab[i].jour, tab[i].mois, tab[i].annee);
        sprintf(horaire_str, "%02d:00-%02d:00", tab[i].heure_debut, tab[i].heure_fin);
        
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                          0, tab[i].id_cours,
                          1, tab[i].nom_cours,
                          2, tab[i].type_activite,
                          3, date_str,
                          4, horaire_str,
                          5, get_niveau_nom(tab[i].niveau),
                          6, tab[i].places_max,
                          7, tab[i].centre,
                          -1);
    }
}

// Function to refresh member courses treeview (shows only courses with available places)
void refresh_member_courses_treeview(GtkWidget *treeview) {
    if (!treeview) return;
    
    Cours tab[MAX_COURS];
    int n = lire_tous_les_cours(COURS_FILE_PATH, tab, MAX_COURS);
    
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    GtkListStore *store;
    
    if (model == NULL) {
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        
        store = gtk_list_store_new(8, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_INT, G_TYPE_STRING);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", 0, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 1, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 2, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", 3, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Horaire", renderer, "text", 4, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Niveau", renderer, "text", 5, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Places", renderer, "text", 6, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Centre", renderer, "text", 7, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
        
        gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
        g_object_unref(store);
    } else {
        store = GTK_LIST_STORE(model);
        gtk_list_store_clear(store);
    }
    
    GtkTreeIter iter;
    for (int i = 0; i < n; i++) {
        // Only show courses with available places
        if (tab[i].places_max <= 0) continue;
        
        char date_str[50], horaire_str[50];
        sprintf(date_str, "%02d/%02d/%04d", tab[i].jour, tab[i].mois, tab[i].annee);
        sprintf(horaire_str, "%02d:00-%02d:00", tab[i].heure_debut, tab[i].heure_fin);
        
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                          0, tab[i].id_cours,
                          1, tab[i].nom_cours,
                          2, tab[i].type_activite,
                          3, date_str,
                          4, horaire_str,
                          5, get_niveau_nom(tab[i].niveau),
                          6, tab[i].places_max,
                          7, tab[i].centre,
                          -1);
    }
}
static int lire_centres(const char *filename, Centre *tab, int max_size) {
    FILE *f = fopen(filename, "r");
    int count = 0;
    
    if (f == NULL) {
        g_warning("Cannot open centres file: %s", filename);
        return 0;
    }
    
    char line[512];
    while (count < max_size && fgets(line, sizeof(line), f)) {
        // Skip empty lines
        if (line[0] == '\n' || line[0] == '\0') continue;
        
        int result = sscanf(line, "%[^,],%[^,],%[^,],%d,%d,%d,%d,%d,%[^\n]",
                  tab[count].id,
                  tab[count].nom,
                  tab[count].type,
                  &tab[count].capacite,
                  &tab[count].ouvert,
                  &tab[count].parking,
                  &tab[count].wifi,
                  &tab[count].cafeteria,
                  tab[count].ville);
        
        if (result == 9) {
            count++;
        }
    }
    
    fclose(f);
    return count;
}
// Function to setup centres treeview columns
static void setup_centres_treeview(GtkTreeView *treeview) {
    if (!treeview) return;
    
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    if (model == NULL) {
        GtkListStore *store = gtk_list_store_new(6, 
            G_TYPE_STRING,  // ID
            G_TYPE_STRING,  // Nom
            G_TYPE_STRING,  // Type
            G_TYPE_INT,     // Capacité
            G_TYPE_STRING,  // Statut (Ouvert/Fermé)
            G_TYPE_STRING   // Ville
        );
        gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
        g_object_unref(store);
        
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", 0, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(treeview, column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 1, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(treeview, column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 2, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(treeview, column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Capacité", renderer, "text", 3, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(treeview, column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Statut", renderer, "text", 4, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(treeview, column);
        
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Ville", renderer, "text", 5, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(treeview, column);
    }
}

// Function to refresh admin centres treeview
void refresh_admin_centres_treeview(GtkWidget *treeview) {
    if (!treeview) return;
    
    Centre tab[MAX_CENTRES];
    int n = lire_centres(CENTRE_FILE_PATH, tab, MAX_CENTRES);
    
    setup_centres_treeview(GTK_TREE_VIEW(treeview));
    
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    GtkListStore *store = GTK_LIST_STORE(model);
    gtk_list_store_clear(store);
    
    GtkTreeIter iter;
    for (int i = 0; i < n; i++) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            0, tab[i].id,
            1, tab[i].nom,
            2, tab[i].type,
            3, tab[i].capacite,
            4, tab[i].ouvert ? "Ouvert" : "Fermé",
            5, tab[i].ville,
            -1);
    }
}

// Function to refresh trainer centres treeview
void refresh_trainer_centres_treeview(GtkWidget *treeview) {
    if (!treeview) return;
    
    Centre tab[MAX_CENTRES];
    int n = lire_centres(CENTRE_FILE_PATH, tab, MAX_CENTRES);
    
    setup_centres_treeview(GTK_TREE_VIEW(treeview));
    
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    GtkListStore *store = GTK_LIST_STORE(model);
    gtk_list_store_clear(store);
    
    GtkTreeIter iter;
    for (int i = 0; i < n; i++) {
        // Only show open centres for trainers
        if (tab[i].ouvert) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                0, tab[i].id,
                1, tab[i].nom,
                2, tab[i].type,
                3, tab[i].capacite,
                4, "Ouvert",  // Always "Ouvert" since we filtered
                5, tab[i].ville,
                -1);
        }
    }
}

// Function to populate centre types combobox for admin
void refresh_centre_types_combobox(GtkWidget *combobox, const char *filename) {
    if (!combobox) return;
    
    Centre tab[MAX_CENTRES];
    int n = lire_centres(filename, tab, MAX_CENTRES);
    
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
    if (model != NULL) {
        gtk_list_store_clear(GTK_LIST_STORE(model));
    }
    
    // Set up cell renderer
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_clear(GTK_CELL_LAYOUT(combobox));
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combobox), renderer, "text", 0, NULL);
    
    // Create new model if needed
    if (model == NULL) {
        GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
        gtk_combo_box_set_model(GTK_COMBO_BOX(combobox), GTK_TREE_MODEL(store));
        g_object_unref(store);
        model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
    }
    
    GtkListStore *store = GTK_LIST_STORE(model);
    GtkTreeIter iter;
    
    // Add "Tous les types" option
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Tous les types", -1);
    
    // Track unique types
    char types[MAX_CENTRES][MAX_STR];
    int nb_types = 0;
    
    for (int i = 0; i < n; i++) {
        if (tab[i].type[0] == '\0') continue;
        
        int exists = 0;
        for (int j = 0; j < nb_types; j++) {
            if (strcmp(types[j], tab[i].type) == 0) {
                exists = 1;
                break;
            }
        }
        
        if (!exists && nb_types < MAX_CENTRES) {
            strcpy(types[nb_types], tab[i].type);
            nb_types++;
            
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, tab[i].type, -1);
        }
    }
    
    gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
}

// Function to populate centre types combobox for trainer
void refresh_trainer_centre_types_combobox(GtkWidget *combobox, const char *filename) {
    if (!combobox) return;
    
    Centre tab[MAX_CENTRES];
    int n = lire_centres(filename, tab, MAX_CENTRES);
    
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
    if (model != NULL) {
        gtk_list_store_clear(GTK_LIST_STORE(model));
    }
    
    // Set up cell renderer
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_clear(GTK_CELL_LAYOUT(combobox));
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combobox), renderer, "text", 0, NULL);
    
    // Create new model if needed
    if (model == NULL) {
        GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
        gtk_combo_box_set_model(GTK_COMBO_BOX(combobox), GTK_TREE_MODEL(store));
        g_object_unref(store);
        model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
    }
    
    GtkListStore *store = GTK_LIST_STORE(model);
    GtkTreeIter iter;
    
    // Add "Tous les types" option
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Tous les types", -1);
    
    // Track unique types from OPEN centres only
    char types[MAX_CENTRES][MAX_STR];
    int nb_types = 0;
    
    for (int i = 0; i < n; i++) {
        // Only consider open centres for trainers
        if (!tab[i].ouvert || tab[i].type[0] == '\0') continue;
        
        int exists = 0;
        for (int j = 0; j < nb_types; j++) {
            if (strcmp(types[j], tab[i].type) == 0) {
                exists = 1;
                break;
            }
        }
        
        if (!exists && nb_types < MAX_CENTRES) {
            strcpy(types[nb_types], tab[i].type);
            nb_types++;
            
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, tab[i].type, -1);
        }
    }
    
    gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
}
static int lire_tous_les_events(const char *filename, evenement *tab, int max_size) {
 
}

// Setup event treeview columns
static void setup_events_treeview(GtkTreeView *treeview) {
    if (!treeview) return;
    
    // Check if already setup by getting the columns list
    GList *columns = gtk_tree_view_get_columns(treeview);
    if (columns != NULL) {
        g_list_free(columns);
        return; // Already setup
    }
    
    // Create model
    GtkListStore *store = gtk_list_store_new(6,
        G_TYPE_STRING,      // ID
        G_TYPE_STRING,   // Nom
        G_TYPE_STRING,   // Type
        G_TYPE_STRING,   // Centre
        G_TYPE_STRING,   // Date (formatted string)
        G_TYPE_STRING    // Type Acces (Gratuit/Payant)
    );
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
    g_object_unref(store);
    
    // Add columns
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 1, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 2, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Centre", renderer, "text", 3, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", 4, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Accès", renderer, "text", 5, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
}

// Refresh admin events treeview (shows all events)
void refresh_admin_events_treeview(GtkWidget *treeview) {
 
}

static int lire_tous_les_entraineurs(const char *filename, Entraineur *tab, int max_size) {
    FILE *f = fopen(filename, "r");
    int count = 0;
    
    if (f == NULL) {
        g_warning("Cannot open trainers file: %s", filename);
        return 0;
    }
    
    char line[512];
    while (count < max_size && fgets(line, sizeof(line), f)) {
        // Skip empty lines
        if (line[0] == '\n' || line[0] == '\0') continue;
        
        int result = sscanf(line, "%d,%99[^,],%99[^,],%d,%99[^,],%d/%d/%d,%d",
                  &tab[count].CIN_trainer,
                  tab[count].Nom_trainer,
                  tab[count].Prenom_trainer,
                  &tab[count].sexe_trainer,
                  tab[count].specialite,
                  &tab[count].jour,
                  &tab[count].mois,
                  &tab[count].annee,
                  &tab[count].train_priv);
        
        if (result == 9) {
            count++;
        }
    }
    
    fclose(f);
    return count;
}
static void setup_trainers_treeview(GtkTreeView *treeview) {
    if (!treeview) return;
    
    // Check if already setup
    GList *columns = gtk_tree_view_get_columns(treeview);
    if (columns != NULL) {
        g_list_free(columns);
        return;
    }
    
    // Create model with 7 columns
    GtkListStore *store = gtk_list_store_new(7,
        G_TYPE_INT,      // CIN
        G_TYPE_STRING,   // Nom
        G_TYPE_STRING,   // Prenom
        G_TYPE_STRING,   // Sexe
        G_TYPE_STRING,   // Specialite
        G_TYPE_STRING,   // Date naissance
        G_TYPE_STRING    // Type (Privé/Public)
    );
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
    g_object_unref(store);
    
    // Add columns
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("CIN", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 1, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Prénom", renderer, "text", 2, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Sexe", renderer, "text", 3, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Spécialité", renderer, "text", 4, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Date Naissance", renderer, "text", 5, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 6, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
}

void refresh_admin_trainers_treeview(GtkWidget *treeview) {
    if (!treeview) return;
    
    Entraineur tab[1000];
    int n = lire_tous_les_entraineurs(ENTRAINEUR_FILE_PATH, tab, 1000);
    
    setup_trainers_treeview(GTK_TREE_VIEW(treeview));
    
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    GtkListStore *store = GTK_LIST_STORE(model);
    gtk_list_store_clear(store);
    
    GtkTreeIter iter;
    for (int i = 0; i < n; i++) {
        char date_str[20];
        snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d", 
                 tab[i].jour, tab[i].mois, tab[i].annee);
        
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            0, tab[i].CIN_trainer,
            1, tab[i].Nom_trainer,
            2, tab[i].Prenom_trainer,
            3, tab[i].sexe_trainer ? "Femme" : "Homme",
            4, tab[i].specialite,
            5, date_str,
            6, tab[i].train_priv ? "Privé" : "Public",
            -1);
    }
}


// Helper function to read all membres from file
static int lire_tous_les_membres(const char *filename, Membre *tab, int max_size) {
    FILE *f = fopen(filename, "r");
    int count = 0;
    
    if (f == NULL) {
        g_warning("Cannot open membres file: %s", filename);
        return 0;
    }
    
    char line[512];
    while (count < max_size && fgets(line, sizeof(line), f)) {
        // Skip empty lines
        if (line[0] == '\n' || line[0] == '\0') continue;
        
        int result = sscanf(line, "%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^\n]",
                  tab[count].cin,
                  tab[count].nom,
                  tab[count].prenom,
                  tab[count].sexe,
                  tab[count].telephone,
                  tab[count].date_inscription);
        
        if (result == 6) {
            count++;
        }
    }
    
    fclose(f);
    return count;
}

// Setup membres treeview columns
static void setup_membres_treeview(GtkTreeView *treeview) {
    if (!treeview) return;
    
    // Check if already setup
    GList *columns = gtk_tree_view_get_columns(treeview);
    if (columns != NULL) {
        g_list_free(columns);
        return;
    }
    
    // Create model with 6 columns
    GtkListStore *store = gtk_list_store_new(6,
        G_TYPE_STRING,   // CIN
        G_TYPE_STRING,   // Nom
        G_TYPE_STRING,   // Prenom
        G_TYPE_STRING,   // Sexe
        G_TYPE_STRING,   // Telephone
        G_TYPE_STRING    // Date inscription
    );
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
    g_object_unref(store);
    
    // Add columns
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("CIN", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 1, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Prénom", renderer, "text", 2, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Sexe", renderer, "text", 3, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Téléphone", renderer, "text", 4, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Date Inscription", renderer, "text", 5, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
}

// Refresh admin membres treeview
void refresh_admin_membres_treeview(GtkWidget *treeview) {
    if (!treeview) return;
    
    Membre tab[1000];
    int n = lire_tous_les_membres(MEMBRE_FILE_PATH, tab, 1000);
    
    setup_membres_treeview(GTK_TREE_VIEW(treeview));
    
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    GtkListStore *store = GTK_LIST_STORE(model);
    gtk_list_store_clear(store);
    
    GtkTreeIter iter;
    for (int i = 0; i < n; i++) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            0, tab[i].cin,
            1, tab[i].nom,
            2, tab[i].prenom,
            3, tab[i].sexe,
            4, tab[i].telephone,
            5, tab[i].date_inscription,
            -1);
    }
}

// Helper function to check if private trainer exists in file
static int entraineur_prive_existe(int cin, EntraineurPrive *ep) {
    FILE *f = fopen(ENTRAINEUR_PRIVE_FILE_PATH, "r");
    if (f == NULL) return 0;
    
    char line[512];
    int found = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        
        int file_cin;
        char file_nom[100], file_prenom[100], file_spec[100], file_jours[20];
        file_jours[0] = '\0'; // Initialize to empty
        
        int result = sscanf(line, "%d|%[^|]|%[^|]|%[^|]|%[^\n]", 
                   &file_cin, file_nom, file_prenom, file_spec, file_jours);
        
        // Check both 5 fields and 4 fields format
        if (result >= 4 && file_cin == cin) {
            found = 1;
            if (ep != NULL) {
                ep->cin = file_cin;
                strcpy(ep->nom, file_nom);
                strcpy(ep->prenom, file_prenom);
                strcpy(ep->specialite, file_spec);
                strcpy(ep->jours_disponibles, file_jours);
            }
            break;
        }
    }
    
    fclose(f);
    return found;
}

// Helper function to add private trainer to file
static void ajouter_entraineur_prive(int cin, const char *nom, const char *prenom, const char *specialite) {
    FILE *f = fopen(ENTRAINEUR_PRIVE_FILE_PATH, "a");
    if (f == NULL) {
        g_warning("Cannot open private trainers file for writing: %s", ENTRAINEUR_PRIVE_FILE_PATH);
        return;
    }
    
    // Add with CIN and all 7 days available: L M Me J V S D
    fprintf(f, "%d|%s|%s|%s|L M Me J V S\n", cin, nom, prenom, specialite);
    fclose(f);
}

// Helper function to synchronize private trainers
static void synchroniser_entraineurs_prives() {
    // Read all trainers from entraineurs.txt
    Entraineur tab[1000];
    int n = lire_tous_les_entraineurs(ENTRAINEUR_FILE_PATH, tab, 1000);
    
    // Create file if it doesn't exist
    FILE *f = fopen(ENTRAINEUR_PRIVE_FILE_PATH, "a+");
    if (f != NULL) {
        fclose(f);
    }
    
    // Check each private trainer
    for (int i = 0; i < n; i++) {
        if (tab[i].train_priv == 1) { // Private trainer
            if (!entraineur_prive_existe(tab[i].CIN_trainer, NULL)) {
                ajouter_entraineur_prive(tab[i].CIN_trainer, tab[i].Nom_trainer, 
                                        tab[i].Prenom_trainer, tab[i].specialite);
            }
        }
    }
}

// Helper function to read private trainers with available days
static int lire_entraineurs_prives_disponibles(EntraineurPrive *tab, int max_size) {
    synchroniser_entraineurs_prives();
    
    FILE *f = fopen(ENTRAINEUR_PRIVE_FILE_PATH, "r");
    int count = 0;
    
    if (f == NULL) {
        g_warning("Cannot open private trainers file: %s", ENTRAINEUR_PRIVE_FILE_PATH);
        return 0;
    }
    
    char line[512];
    while (count < max_size && fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        
        EntraineurPrive ep;
        // Initialize jours_disponibles to empty
        ep.jours_disponibles[0] = '\0';
        
        int result = sscanf(line, "%d|%[^|]|%[^|]|%[^|]|%[^\n]", 
                   &ep.cin, ep.nom, ep.prenom, ep.specialite, ep.jours_disponibles);
        
        // Accept both 5 fields (with jours) or 4 fields (without jours)
        if (result >= 4) {
            // Trim whitespace from jours_disponibles
            char *start = ep.jours_disponibles;
            char *end;
            
            // Trim leading spaces
            while (*start == ' ') start++;
            
            // Trim trailing spaces
            end = start + strlen(start) - 1;
            while (end > start && (*end == ' ' || *end == '\n' || *end == '\r')) end--;
            *(end + 1) = '\0';
            
            // Move trimmed string to beginning
            if (start != ep.jours_disponibles) {
                memmove(ep.jours_disponibles, start, strlen(start) + 1);
            }
            
            // Only add if they have at least one available day
            if (strlen(ep.jours_disponibles) > 0) {
                tab[count] = ep;
                count++;
            }
        }
    }
    
    fclose(f);
    return count;
}
// Setup private trainers treeview columns
static void setup_entraineurs_prives_treeview(GtkTreeView *treeview) {
    if (!treeview) return;
    
    // Check if already setup
    GList *columns = gtk_tree_view_get_columns(treeview);
    if (columns != NULL) {
        g_list_free(columns);
        return;
    }
    
    // Create model with 4 columns
    GtkListStore *store = gtk_list_store_new(4,
        G_TYPE_STRING,   // Nom
        G_TYPE_STRING,   // Prenom
        G_TYPE_STRING,   // Specialite
        G_TYPE_STRING    // Jours Disponibles
    );
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
    g_object_unref(store);
    
    // Add columns
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Prénom", renderer, "text", 1, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Spécialité", renderer, "text", 2, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Jours Disponibles", renderer, "text", 3, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(treeview, column);
}

// Refresh membre entraineurs prives treeview (only shows trainers with available days)
void refresh_membre_entraineurs_prives_treeview(GtkWidget *treeview) {
    if (!treeview) return;
    
    EntraineurPrive tab[1000];
    int n = lire_entraineurs_prives_disponibles(tab, 1000);
    
    setup_entraineurs_prives_treeview(GTK_TREE_VIEW(treeview));
    
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    GtkListStore *store = GTK_LIST_STORE(model);
    gtk_list_store_clear(store);
    
    GtkTreeIter iter;
    for (int i = 0; i < n; i++) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            0, tab[i].nom,
            1, tab[i].prenom,
            2, tab[i].specialite,
            3, tab[i].jours_disponibles,
            -1);
    }
}

/*****************************************************/

/******************* EVENT FUNCTIONS *******************/

/******************* Helper Function for Auto-Increment *******************/

int get_next_event_id() {
    evenement ev;
    int max_id = 0;
    FILE *f = fopen("evenement.txt", "r");
    
    if (f) {
        while (fscanf(f, "%d %299s %99s %199s %d/%d/%d %19s\n",
                      &ev.id_event, ev.nom_event, ev.type_event, ev.centre,
                      &ev.date_e.j, &ev.date_e.m, &ev.date_e.a, ev.statut) != EOF) {
            if (ev.id_event > max_id) {
                max_id = ev.id_event;
            }
        }
        fclose(f);
    }
    return max_id + 1;
}

/******************* CRUD Functions for Events *******************/

void ajouter_evenement(evenement ev) {
    FILE *f = fopen("evenement.txt", "a");
    if (f) {
        // Auto-increment the ID
        ev.id_event = get_next_event_id();
        
        fprintf(f, "%d %s %s %s %02d/%02d/%04d %s\n",
                ev.id_event, ev.nom_event, ev.type_event, ev.centre,
                ev.date_e.j, ev.date_e.m, ev.date_e.a, ev.statut);
        fflush(f);
        fclose(f);
    }
}

int evenement_existe(int id_event) {
    evenement ev;
    FILE *f = fopen("evenement.txt", "r");
    if (!f) return 0;

    while (fscanf(f, "%d %299s %99s %199s %d/%d/%d %19s\n",
                  &ev.id_event, ev.nom_event, ev.type_event, ev.centre,
                  &ev.date_e.j, &ev.date_e.m, &ev.date_e.a, ev.statut) != EOF) {
        if (ev.id_event == id_event) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}
/******************************************************************************/
void supprimer_evenement(int id_event) {
    evenement ev;
    FILE *f = fopen("evenement.txt", "r");
    FILE *temp = fopen("temp.txt", "w");

    if (f && temp) {
        while (fscanf(f, "%d %299s %99s %199s %d/%d/%d %19s\n",
                      &ev.id_event, ev.nom_event, ev.type_event, ev.centre,
                      &ev.date_e.j, &ev.date_e.m, &ev.date_e.a, ev.statut) != EOF) {
            if (ev.id_event != id_event) {
                fprintf(temp, "%d %s %s %s %02d/%02d/%04d %s\n",
                        ev.id_event, ev.nom_event, ev.type_event, ev.centre,
                        ev.date_e.j, ev.date_e.m, ev.date_e.a, ev.statut);
            }
        }
    }
    if (f) fclose(f);
    if (temp) fclose(temp);
    remove("evenement.txt");
    rename("temp.txt", "evenement.txt");
}

void modifier_evenement(evenement ev) {
    evenement temp_ev;
    FILE *f = fopen("evenement.txt", "r");
    FILE *temp = fopen("temp.txt", "w");

    if (!f || !temp) {
        printf("Erreur : Impossible d'ouvrir le fichier pour modification.\n");
        if (f) fclose(f);
        if (temp) fclose(temp);
        return;
    }

    while (fscanf(f, "%d %299s %99s %199s %d/%d/%d %19s\n",
                  &temp_ev.id_event, temp_ev.nom_event, temp_ev.type_event, temp_ev.centre,
                  &temp_ev.date_e.j, &temp_ev.date_e.m, &temp_ev.date_e.a, temp_ev.statut) != EOF) {
        if (temp_ev.id_event == ev.id_event) {
            fprintf(temp, "%d %s %s %s %02d/%02d/%04d %s\n",
                    ev.id_event, ev.nom_event, ev.type_event, ev.centre,
                    ev.date_e.j, ev.date_e.m, ev.date_e.a, ev.statut);
        } else {
            fprintf(temp, "%d %s %s %s %02d/%02d/%04d %s\n",
                    temp_ev.id_event, temp_ev.nom_event, temp_ev.type_event, temp_ev.centre,
                    temp_ev.date_e.j, temp_ev.date_e.m, temp_ev.date_e.a, temp_ev.statut);
        }
    }

    fclose(f);
    fclose(temp);

    remove("evenement.txt");
    rename("temp.txt", "evenement.txt");
}

evenement trouver_evenement(int id_event) {
    evenement ev;
    FILE *f = fopen("evenement.txt", "r");

    if (f) {
        while (fscanf(f, "%d %299s %99s %199s %d/%d/%d %19s\n",
                      &ev.id_event, ev.nom_event, ev.type_event, ev.centre,
                      &ev.date_e.j, &ev.date_e.m, &ev.date_e.a, ev.statut) != EOF) {
            if (ev.id_event == id_event) {
                fclose(f);
                return ev;
            }
        }
        fclose(f);
    }
    evenement empty = {0, "", "", "", {0, 0, 0}, ""};
    return empty;
}

/******************* Display Function for Events *******************/

enum {
    ID_EVENT,
    NOM_EVENT,
    TYPE_EVENT,
    CENTRE,
    DATE_EVENT,
    STATUT,
    COLUMNS_EVENT
};

void afficher_evenements(GtkWidget *list) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeIter iter;
    GtkListStore *store;

    int id_event;
    char nom_event[300], type_event[100], centre[200], statut[20];
    int jour, mois, annee;
    char date[12];

    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));

    if (store == NULL) {
        // Create columns for the TreeView
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", ID_EVENT, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", NOM_EVENT, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", TYPE_EVENT, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Centre", renderer, "text", CENTRE, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", DATE_EVENT, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Statut", renderer, "text", STATUT, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        // Create ListStore with appropriate types
        store = gtk_list_store_new(COLUMNS_EVENT,
                                   G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        
        // Set the model ONCE during initialization
        gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));
        g_object_unref(store);
    }

    // Clear the store
    gtk_list_store_clear(store);

    FILE *f = fopen("evenement.txt", "r");
    if (!f) {
        printf("Erreur : impossible d'ouvrir le fichier evenement.txt.\n");
        return;
    }

    // Read data from file and populate the ListStore
    while (fscanf(f, "%d %299s %99s %199s %d/%d/%d %19s\n",
                  &id_event, nom_event, type_event, centre,
                  &jour, &mois, &annee, statut) != EOF) {
        snprintf(date, sizeof(date), "%02d/%02d/%04d", jour, mois, annee);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           ID_EVENT, id_event,
                           NOM_EVENT, nom_event,
                           TYPE_EVENT, type_event,
                           CENTRE, centre,
                           DATE_EVENT, date,
                           STATUT, statut,
                           -1);
    }

    fclose(f);
}

/******************* Search Function for Events *******************/

int rechercher_evenements(const char *critere, GtkWidget *treeview) {
    evenement ev;
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
        column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", ID_EVENT, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", NOM_EVENT, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", TYPE_EVENT, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Centre", renderer, "text", CENTRE, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", DATE_EVENT, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Statut", renderer, "text", STATUT, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        store = gtk_list_store_new(COLUMNS_EVENT,
                                   G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        
        // Set the model ONCE during initialization
        gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
        g_object_unref(store);
    }
    
    // Clear the store
    gtk_list_store_clear(store);
    
    // Open file
    FILE *f = fopen("CENTRE_FILE_PATH", "r");
    if (!f) {
        printf("Erreur : impossible d'ouvrir le fichier evenement.txt.\n");
        return 0;
    }
    
    // Read and search
    while (fscanf(f, "%d %299s %99s %199s %d/%d/%d %19s\n",
                  &ev.id_event, ev.nom_event, ev.type_event, ev.centre,
                  &ev.date_e.j, &ev.date_e.m, &ev.date_e.a, ev.statut) != EOF) {
        
        // Convert all fields to lowercase for comparison
        char nom_lower[300], type_lower[100], centre_lower[200], statut_lower[20];
        strcpy(nom_lower, ev.nom_event);
        strcpy(type_lower, ev.type_event);
        strcpy(centre_lower, ev.centre);
        strcpy(statut_lower, ev.statut);
        
        for (int i = 0; nom_lower[i]; i++) nom_lower[i] = tolower(nom_lower[i]);
        for (int i = 0; type_lower[i]; i++) type_lower[i] = tolower(type_lower[i]);
        for (int i = 0; centre_lower[i]; i++) centre_lower[i] = tolower(centre_lower[i]);
        for (int i = 0; statut_lower[i]; i++) statut_lower[i] = tolower(statut_lower[i]);
        
        // Check if criteria matches any field (case-insensitive)
        if (strstr(nom_lower, critere_lower) != NULL ||
            strstr(type_lower, critere_lower) != NULL ||
            strstr(centre_lower, critere_lower) != NULL ||
            strstr(statut_lower, critere_lower) != NULL) {
            
            found++;
            
            // Format date
            char date_str[12];
            snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d", 
                    ev.date_e.j, ev.date_e.m, ev.date_e.a);
            
            // Add to store
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                              ID_EVENT, ev.id_event,
                              NOM_EVENT, ev.nom_event,
                              TYPE_EVENT, ev.type_event,
                              CENTRE, ev.centre,
                              DATE_EVENT, date_str,
                              STATUT, ev.statut,
                              -1);
        }
    }
    
    fclose(f);
    
    return found;
}

/******************* ENROLLMENT FUNCTIONS *******************/

/******************* Helper Function for Enrollment Auto-Increment *******************/

int get_next_inscription_id() {
    inscription_event insc;
    int max_id = 0;
    FILE *f = fopen("inscription_event.txt", "r");
    
    if (f) {
        while (fscanf(f, "%d %19s %d %299s %99s %199s %d/%d/%d\n",
                      &insc.id_inscription, insc.cin_membre, &insc.id_event,
                      insc.nom_event, insc.type_event, insc.centre,
                      &insc.date_e.j, &insc.date_e.m, &insc.date_e.a) != EOF) {
            if (insc.id_inscription > max_id) {
                max_id = insc.id_inscription;
            }
        }
        fclose(f);
    }
    return max_id + 1;
}

/******************* CRUD Functions for memebers *******************/

void ajouter_inscription_event(inscription_event insc) {
    FILE *f = fopen("inscription_event.txt", "a");
    if (f) {
        insc.id_inscription = get_next_inscription_id();
        
        fprintf(f, "%d %s %d %s %s %s %02d/%02d/%04d\n",
                insc.id_inscription, insc.cin_membre, insc.id_event,
                insc.nom_event, insc.type_event, insc.centre,
                insc.date_e.j, insc.date_e.m, insc.date_e.a);
        fflush(f);
        fclose(f);
    }
}

int inscription_existe(const char *cin_membre, int id_event) {
    inscription_event insc;
    FILE *f = fopen("inscription_event.txt", "r");
    if (!f) return 0;

    while (fscanf(f, "%d %19s %d %299s %99s %199s %d/%d/%d\n",
                  &insc.id_inscription, insc.cin_membre, &insc.id_event,
                  insc.nom_event, insc.type_event, insc.centre,
                  &insc.date_e.j, &insc.date_e.m, &insc.date_e.a) != EOF) {
        if (strcmp(insc.cin_membre, cin_membre) == 0 && insc.id_event == id_event) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void supprimer_inscription_event(int id_inscription) {
    inscription_event insc;
    FILE *f = fopen("inscription_event.txt", "r");
    FILE *temp = fopen("temp_insc.txt", "w");

    if (f && temp) {
        while (fscanf(f, "%d %19s %d %299s %99s %199s %d/%d/%d\n",
                      &insc.id_inscription, insc.cin_membre, &insc.id_event,
                      insc.nom_event, insc.type_event, insc.centre,
                      &insc.date_e.j, &insc.date_e.m, &insc.date_e.a) != EOF) {
            if (insc.id_inscription != id_inscription) {
                fprintf(temp, "%d %s %d %s %s %s %02d/%02d/%04d\n",
                        insc.id_inscription, insc.cin_membre, insc.id_event,
                        insc.nom_event, insc.type_event, insc.centre,
                        insc.date_e.j, insc.date_e.m, insc.date_e.a);
            }
        }
    }
    if (f) fclose(f);
    if (temp) fclose(temp);
    remove("inscription_event.txt");
    rename("temp_insc.txt", "inscription_event.txt");
}

/******************* Display Function for Member Enrollments *******************/

enum {
    ID_INSC,
    ID_EVENT_INSC,
    NOM_EVENT_INSC,
    TYPE_EVENT_INSC,
    CENTRE_INSC,
    DATE_EVENT_INSC,
    COLUMNS_INSC
};

void afficher_inscriptions_membre(GtkWidget *list, const char *cin_membre) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeIter iter;
    GtkListStore *store;

    int id_inscription, id_event, jour, mois, annee;
    char cin[20], nom_event[300], type_event[100], centre[200];
    char date[12];

    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));

    if (store == NULL) {
        // Create columns
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID Inscription", renderer, "text", ID_INSC, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID Event", renderer, "text", ID_EVENT_INSC, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", NOM_EVENT_INSC, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", TYPE_EVENT_INSC, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Centre", renderer, "text", CENTRE_INSC, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", DATE_EVENT_INSC, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

        store = gtk_list_store_new(COLUMNS_INSC,
                                   G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        
        gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));
        g_object_unref(store);
    }

    gtk_list_store_clear(store);

    FILE *f = fopen("inscription_event.txt", "r");
    if (!f) {
        printf("Erreur : impossible d'ouvrir inscription_event.txt.\n");
        return;
    }

    while (fscanf(f, "%d %19s %d %299s %99s %199s %d/%d/%d\n",
                  &id_inscription, cin, &id_event,
                  nom_event, type_event, centre,
                  &jour, &mois, &annee) != EOF) {
        
        if (strcmp(cin, cin_membre) == 0) {
            snprintf(date, sizeof(date), "%02d/%02d/%04d", jour, mois, annee);

            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               ID_INSC, id_inscription,
                               ID_EVENT_INSC, id_event,
                               NOM_EVENT_INSC, nom_event,
                               TYPE_EVENT_INSC, type_event,
                               CENTRE_INSC, centre,
                               DATE_EVENT_INSC, date,
                               -1);
        }
    }

    fclose(f);
}

/******************* Search Function for Member Enrollments *******************/

int rechercher_mes_inscriptions(const char *critere, GtkWidget *treeview, const char *cin_membre) {
    inscription_event insc;
    char critere_lower[600];
    int found = 0;
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeIter iter;
    GtkListStore *store;
    
    if (!treeview) return 0;
    
    strcpy(critere_lower, critere);
    for (int i = 0; critere_lower[i]; i++) {
        critere_lower[i] = tolower(critere_lower[i]);
    }
    
    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));
    
    if (store == NULL) {
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID Inscription", renderer, "text", ID_INSC, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID Event", renderer, "text", ID_EVENT_INSC, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", NOM_EVENT_INSC, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", TYPE_EVENT_INSC, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Centre", renderer, "text", CENTRE_INSC, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", DATE_EVENT_INSC, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        store = gtk_list_store_new(COLUMNS_INSC,
                                   G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        
        gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
        g_object_unref(store);
    }
    
    gtk_list_store_clear(store);
    
    FILE *f = fopen("inscription_event.txt", "r");
    if (!f) return 0;
    
    while (fscanf(f, "%d %19s %d %299s %99s %199s %d/%d/%d\n",
                  &insc.id_inscription, insc.cin_membre, &insc.id_event,
                  insc.nom_event, insc.type_event, insc.centre,
                  &insc.date_e.j, &insc.date_e.m, &insc.date_e.a) != EOF) {
        
        if (strcmp(insc.cin_membre, cin_membre) != 0) continue;
        
        char nom_lower[300], type_lower[100], centre_lower[200];
        strcpy(nom_lower, insc.nom_event);
        strcpy(type_lower, insc.type_event);
        strcpy(centre_lower, insc.centre);
        
        for (int i = 0; nom_lower[i]; i++) nom_lower[i] = tolower(nom_lower[i]);
        for (int i = 0; type_lower[i]; i++) type_lower[i] = tolower(type_lower[i]);
        for (int i = 0; centre_lower[i]; i++) centre_lower[i] = tolower(centre_lower[i]);
        
        if (strstr(nom_lower, critere_lower) != NULL ||
            strstr(type_lower, critere_lower) != NULL ||
            strstr(centre_lower, critere_lower) != NULL) {
            
            found++;
            
            char date_str[12];
            snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d", 
                    insc.date_e.j, insc.date_e.m, insc.date_e.a);
            
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                              ID_INSC, insc.id_inscription,
                              ID_EVENT_INSC, insc.id_event,
                              NOM_EVENT_INSC, insc.nom_event,
                              TYPE_EVENT_INSC, insc.type_event,
                              CENTRE_INSC, insc.centre,
                              DATE_EVENT_INSC, date_str,
                              -1);
        }
    }
    
    fclose(f);
    return found;
}
/*************************************************/
/**********************************************************************************************************entraineur isncrit chiraz *******************/

int rechercher_cours(const char *filename, const char *id, Cours *res) {
    Cours tab[MAX_COURS];
    int n = lire_tous_les_cours(filename, tab, MAX_COURS);
    for (int i = 0; i < n; ++i) {
        if (strcmp(tab[i].id_cours, id) == 0) {
            *res = tab[i];
            return 1;
        }
    }
    return 0;
}
int entraineur_a_deja_un_cours(const char *cin_trainer) {
    inscription_trainer_cours insc;
    FILE *f = fopen("inscription_trainer_cours.txt", "r");
    if (!f) return 0;

    while (fscanf(f, "%d %19s %99s\n",
                  &insc.id_inscription_trainer,
                  insc.cin_trainer,
                  insc.id_cours) != EOF) {
        
        if (strcmp(insc.cin_trainer, cin_trainer) == 0) {
            fclose(f);
            return 1;  // Already has at least one course
        }
    }
    fclose(f);
    return 0;  // No course yet
}

int cours_deja_avec_entraineur(const char *id_cours) {
    inscription_trainer_cours insc;
    FILE *f = fopen("inscription_trainer_cours.txt", "r");
    if (!f) return 0;

    while (fscanf(f, "%d %19s %99s\n",
                  &insc.id_inscription_trainer,
                  insc.cin_trainer,
                  insc.id_cours) != EOF) {
        if (strcmp(insc.id_cours, id_cours) == 0) {
            fclose(f);
            return 1; // Already has a trainer
        }
    }
    fclose(f);
    return 0;
}

int get_next_inscription_trainer_id() {
    inscription_trainer_cours insc;
    int max_id = 0;
    FILE *f = fopen("inscription_trainer_cours.txt", "r");
    
    if (f) {
        while (fscanf(f, "%d %19s %99s\n",
                      &insc.id_inscription_trainer,
                      insc.cin_trainer,
                      insc.id_cours) != EOF) {
            if (insc.id_inscription_trainer > max_id) {
                max_id = insc.id_inscription_trainer;
            }
        }
        fclose(f);
    }
    return max_id + 1;
}

int trainer_deja_inscrit(const char *cin_trainer, const char *id_cours) {
    inscription_trainer_cours insc;
    FILE *f = fopen("inscription_trainer_cours.txt", "r");
    if (!f) return 0;
    
    while (fscanf(f, "%d %19s %99s\n",
                  &insc.id_inscription_trainer,
                  insc.cin_trainer,
                  insc.id_cours) != EOF) {
        if (strcmp(insc.cin_trainer, cin_trainer) == 0 && 
            strcmp(insc.id_cours, id_cours) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}


int compter_places_prises(const char *id_cours) {
    int count = 0;
    int id_inscr;
    char membre_id[MAX_STR], cours_id[MAX_STR];
    
    FILE *f = fopen("inscription_membre.txt", "r");
    if (!f) return 0;
    
    while (fscanf(f, "%d %99s %99s\n", &id_inscr, membre_id, cours_id) != EOF) {
        if (strcmp(cours_id, id_cours) == 0) {
            count++;
        }
    }
    fclose(f);
    return count;
}


int inscrire_entraineur_cours(const char *cin_trainer, const char *id_cours) {

    if (entraineur_a_deja_un_cours(cin_trainer)) {
        return -6;
    }


    if (trainer_deja_inscrit(cin_trainer, id_cours)) {
        return -1; 
    }
   
    if (cours_deja_avec_entraineur(id_cours)) {
        return -5;
    }
   
    Cours c;
    if (rechercher_cours(COURS_FILE_PATH, id_cours, &c) == 0) {
        return -2; 
    }
   

    FILE *f = fopen("inscription_trainer_cours.txt", "a");
    if (!f) return -4;
   
    int new_id = get_next_inscription_trainer_id();
    fprintf(f, "%d %s %s\n", new_id, cin_trainer, id_cours);
    fclose(f);
   
    return 1; // Success
}


/******************* GTK DISPLAY FUNCTIONS ********************/
enum {
    ID_COURS_COL = 0,
    NOM_COURS_COL,
    TYPE_COL,
    CENTRE_COURS_COL,
    DATE_COURS_COL,
    HEURE_COL,
    NIVEAU_COL,
    PLACES_COL,
    COLUMNS_COURS
};

void afficher_cours_disponibles(GtkWidget *treeview) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeIter iter;
    GtkListStore *store;

    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));

    if (store == NULL) {
        // Create columns
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", ID_COURS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", NOM_COURS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", TYPE_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Centre", renderer, "text", CENTRE_COURS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", DATE_COURS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Heure", renderer, "text", HEURE_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Niveau", renderer, "text", NIVEAU_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Places", renderer, "text", PLACES_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        store = gtk_list_store_new(COLUMNS_COURS,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING);
        gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
        g_object_unref(store);
    }

    gtk_list_store_clear(store);

    Cours tab[MAX_COURS];
    int n = lire_tous_les_cours(COURS_FILE_PATH, tab, MAX_COURS);

    for (int i = 0; i < n; i++) {
        // Skip courses that already have a trainer
        if (cours_deja_avec_entraineur(tab[i].id_cours)) {
            continue;
        }

        // Format date
        char date_str[20];
        snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d",
                tab[i].jour, tab[i].mois, tab[i].annee);

        // Format hours
        char heure_str[20];
        snprintf(heure_str, sizeof(heure_str), "%02d:00-%02d:00",
                tab[i].heure_debut, tab[i].heure_fin);

        // Format level
        const char *niveau_str;
        switch(tab[i].niveau) {
            case 0: niveau_str = "Débutant"; break;
            case 1: niveau_str = "Intermédiaire"; break;
            case 2: niveau_str = "Avancé"; break;
            default: niveau_str = "Inconnu"; break;
        }

        // Calculate available places
        int places_prises = compter_places_prises(tab[i].id_cours);
        int places_dispo = tab[i].places_max - places_prises;
        char places_str[30];
        snprintf(places_str, sizeof(places_str), "%d/%d", places_dispo, tab[i].places_max);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                          ID_COURS_COL, tab[i].id_cours,
                          NOM_COURS_COL, tab[i].nom_cours,
                          TYPE_COL, tab[i].type_activite,
                          CENTRE_COURS_COL, tab[i].centre,
                          DATE_COURS_COL, date_str,
                          HEURE_COL, heure_str,
                          NIVEAU_COL, niveau_str,
                          PLACES_COL, places_str,
                          -1);
    }
}

void afficher_mes_cours(const char *cin_trainer, GtkWidget *treeview) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeIter iter;
    GtkListStore *store;

    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));

    if (store == NULL) {
        // Create columns
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", ID_COURS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", NOM_COURS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", TYPE_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Centre", renderer, "text", CENTRE_COURS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", DATE_COURS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Heure", renderer, "text", HEURE_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Niveau", renderer, "text", NIVEAU_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Places", renderer, "text", PLACES_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        store = gtk_list_store_new(COLUMNS_COURS,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING);
        gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
        g_object_unref(store);
    }

    gtk_list_store_clear(store);

    inscription_trainer_cours insc;
    FILE *f = fopen("inscription_trainer_cours.txt", "r");
    if (!f) return;

    while (fscanf(f, "%d %19s %99s\n",
                  &insc.id_inscription_trainer,
                  insc.cin_trainer,
                  insc.id_cours) != EOF) {
        if (strcmp(insc.cin_trainer, cin_trainer) == 0) {
            Cours c;
            if (rechercher_cours(COURS_FILE_PATH, insc.id_cours, &c) == 1) {
                char date_str[20];
                snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d",
                        c.jour, c.mois, c.annee);

                char heure_str[20];
                snprintf(heure_str, sizeof(heure_str), "%02d:00-%02d:00",
                        c.heure_debut, c.heure_fin);

                const char *niveau_str;
                switch(c.niveau) {
                    case 0: niveau_str = "Débutant"; break;
                    case 1: niveau_str = "Intermédiaire"; break;
                    case 2: niveau_str = "Avancé"; break;
                    default: niveau_str = "Inconnu"; break;
                }

                int places_prises = compter_places_prises(c.id_cours);
                int places_dispo = c.places_max - places_prises;
                char places_str[30];
                snprintf(places_str, sizeof(places_str), "%d/%d", places_dispo, c.places_max);

                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter,
                                  ID_COURS_COL, c.id_cours,
                                  NOM_COURS_COL, c.nom_cours,
                                  TYPE_COL, c.type_activite,
                                  CENTRE_COURS_COL, c.centre,
                                  DATE_COURS_COL, date_str,
                                  HEURE_COL, heure_str,
                                  NIVEAU_COL, niveau_str,
                                  PLACES_COL, places_str,
                                  -1);
            }
        }
    }
    fclose(f);
}

int rechercher_cours_gtk(const char *critere, GtkWidget *treeview) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeIter iter;
    GtkListStore *store;
    int found = 0;

    char critere_lower[300];
    strcpy(critere_lower, critere);
    for (int i = 0; critere_lower[i]; i++) {
        critere_lower[i] = tolower(critere_lower[i]);
    }

    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));

    if (store == NULL) {
        // Create columns
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", ID_COURS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", NOM_COURS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", TYPE_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Centre", renderer, "text", CENTRE_COURS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", DATE_COURS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Heure", renderer, "text", HEURE_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Niveau", renderer, "text", NIVEAU_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Places", renderer, "text", PLACES_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

        store = gtk_list_store_new(COLUMNS_COURS,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING);
        gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
        g_object_unref(store);
    }

    gtk_list_store_clear(store);

    Cours tab[MAX_COURS];
    int n = lire_tous_les_cours(COURS_FILE_PATH, tab, MAX_COURS);

    for (int i = 0; i < n; i++) {
        // Skip courses that already have a trainer
        if (cours_deja_avec_entraineur(tab[i].id_cours)) {
            continue;
        }

        char nom_lower[MAX_STR], type_lower[MAX_STR], centre_lower[MAX_STR];
        strcpy(nom_lower, tab[i].nom_cours);
        strcpy(type_lower, tab[i].type_activite);
        strcpy(centre_lower, tab[i].centre);

        for (int j = 0; nom_lower[j]; j++) nom_lower[j] = tolower(nom_lower[j]);
        for (int j = 0; type_lower[j]; j++) type_lower[j] = tolower(type_lower[j]);
        for (int j = 0; centre_lower[j]; j++) centre_lower[j] = tolower(centre_lower[j]);

        if (strstr(nom_lower, critere_lower) != NULL ||
            strstr(type_lower, critere_lower) != NULL ||
            strstr(centre_lower, critere_lower) != NULL) {
            
            found++;

            char date_str[20];
            snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d",
                    tab[i].jour, tab[i].mois, tab[i].annee);

            char heure_str[20];
            snprintf(heure_str, sizeof(heure_str), "%02d:00-%02d:00",
                    tab[i].heure_debut, tab[i].heure_fin);

            const char *niveau_str;
            switch(tab[i].niveau) {
                case 0: niveau_str = "Débutant"; break;
                case 1: niveau_str = "Intermédiaire"; break;
                case 2: niveau_str = "Avancé"; break;
                default: niveau_str = "Inconnu"; break;
            }

            int places_prises = compter_places_prises(tab[i].id_cours);
            int places_dispo = tab[i].places_max - places_prises;
            char places_str[30];
            snprintf(places_str, sizeof(places_str), "%d/%d", places_dispo, tab[i].places_max);

            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                              ID_COURS_COL, tab[i].id_cours,
                              NOM_COURS_COL, tab[i].nom_cours,
                              TYPE_COL, tab[i].type_activite,
                              CENTRE_COURS_COL, tab[i].centre,
                              DATE_COURS_COL, date_str,
                              HEURE_COL, heure_str,
                              NIVEAU_COL, niveau_str,
                              PLACES_COL, places_str,
                              -1);
        }
    }

    return found;
}

