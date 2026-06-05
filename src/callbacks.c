#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "callbacks.h"
#include "treeview.h"
#include "interface.h"
#include "support.h"
#include "centres.h"
#include "entraineur.h"

static char filtre_type_actuel[100] = "";
static int filtre_cap_min_actuel = 0;
static int filtre_centres_ouverts = 0;
static int tri_par_nom = 1;  // 1=nom, 0=ville

// Variables pour stocker l'ID du centre sélectionné
static char centre_selectionne_id[7] = "";
/**********************************/

/******************* Global Variables for Events ********************/
int status_event = 0;  // 0 = gratuit, 1 = payant
int selected_event_id = 0;
int event_search_mode = 0;
GtkWidget *global_treeview_events = NULL;
GtkWidget *global_treeview_entraineurs = NULL;

/******************* Global Variables for Member Events ********************/
int selected_event_id_membre = 0;
int selected_inscription_id = 0;
int membre_event_search_mode = 0;
int my_event_search_mode = 0;
char current_member_cin[20] = "";

enum {
    ID_EVENT_COL = 0,
    NOM_EVENT_COL = 1,
    TYPE_EVENT_COL = 2,
    CENTRE_COL = 3,
    DATE_EVENT_COL = 4,
    STATUT_COL = 5,
    COLUMNS_EVENT_COL = 6
};

enum {
    ID_INSC_COL = 0,
    ID_EVENT_INSC_COL = 1,
    NOM_EVENT_INSC_COL = 2,
    TYPE_EVENT_INSC_COL = 3,
    CENTRE_INSC_COL = 4,
    DATE_EVENT_INSC_COL = 5,
    COLUMNS_INSC_COL = 6
};

/***********************************************************/
/******************* Global Variableschiraz ********************/
int w = 1;
int z = 1;
int is_modify_mode = 0;



enum {
    ID_E = 0,
    CIN = 1,
    NOM = 2,
    PRENOM = 3,
    SEXE = 4,
    SPECIALITE = 5,
    DATE_NAISSANCE = 6,
    ENTRAINEMENT_PRIVE = 7,
    COLUMNS_E = 8
};

gchar *selected_cin = NULL;
int search_mode = 0;

/******************* MEMBER EVENTS WINDOW - Show Handler ********************/

/**********************************************/
static void appliquer_filtres_et_afficher(GtkWidget *treeview) {
    if (!treeview) return;
    
    Centre tab[MAX_CENTRES];
    int n = lire_centres_filtres(CENTRE_FILE_PATH, tab, MAX_CENTRES,
                                 filtre_type_actuel, filtre_cap_min_actuel,
                                 filtre_centres_ouverts);
    
    // Trier selon le critère actuel
    trier_centres(tab, n, tri_par_nom);
    
    // Afficher dans le treeview
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

// Function to authenticate user
int authenticate_user(const char *username, const char *password, int *user_id) {
    FILE *file = fopen(USER_FILE_PATH, "r");
    if (file == NULL) {
        g_warning("Cannot open user file: %s", USER_FILE_PATH);
        return 0;
    }

    char line[256];
    int role, id;
    char file_username[50], file_password[50];
    int user_found = 0;

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%d %s %s %d", &role, file_username, file_password, &id) == 4) {
            if (strcmp(file_username, username) == 0) {
                user_found = 1;
                if (strcmp(file_password, password) == 0) {
                    *user_id = id;
                    fclose(file);
                    return role;
                } else {
                    fclose(file);
                    return -1;
                }
            }
        }
    }

    fclose(file);
    return 0;
}

// Show message dialog
void show_message(GtkWidget *parent, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(parent),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s", message
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Login button callback
void on_button_login_clicked(GtkWidget *button, gpointer user_data) {
    GtkWidget *username_entry, *password_entry;
    const char *username, *password;
    int user_id = -1;
    int auth_result;

    GtkWidget *login_window = lookup_widget(button, "window_login");

    username_entry = lookup_widget(login_window, "login_username_entry");
    password_entry = lookup_widget(login_window, "login_password_entry");

    username = gtk_entry_get_text(GTK_ENTRY(username_entry));
    password = gtk_entry_get_text(GTK_ENTRY(password_entry));

    if (strlen(username) == 0 || strlen(password) == 0) {
        show_message(login_window, "Please enter both username and password!");
        return;
    }

    auth_result = authenticate_user(username, password, &user_id);

    if (auth_result == 0) {
        show_message(login_window, "Wrong username!");
        gtk_entry_set_text(GTK_ENTRY(password_entry), "");
    } else if (auth_result == -1) {
        show_message(login_window, "Wrong password for this username!");
        gtk_entry_set_text(GTK_ENTRY(password_entry), "");
    } else {
        gtk_widget_hide(window_login);
        
        gtk_entry_set_text(GTK_ENTRY(username_entry), "");
        gtk_entry_set_text(GTK_ENTRY(password_entry), "");

        switch (auth_result) {
            case 1:
                if (window_espace_admin == NULL) {
                    window_espace_admin = create_window_espace_admin();
                }
                gtk_widget_show(window_espace_admin);
                break;
            
            case 2:
                current_trainer_id = user_id;
                sprintf(current_trainer_id_str, "%d", user_id);
                if (window_espace_entraineur == NULL) {
                    window_espace_entraineur = create_window_espace_entraineur();
                }
                gtk_widget_show(window_espace_entraineur);
                break;
            
            case 3:
                current_membre_id = user_id;
                sprintf(current_membre_id_str, "%d", user_id);
                if (window_espace_membres == NULL) {
                    window_espace_membres = create_window_espace_membres();
                }
                gtk_widget_show(window_espace_membres);
                break;
            
            default:
                show_message(login_window, "Invalid role!");
                gtk_widget_show(window_login);
                break;
        }
    }
}

// Logout admin callback
void on_button_logout_admin_clicked(GtkWidget *button, gpointer user_data) {
    gtk_widget_hide(window_espace_admin);
    gtk_widget_show(window_login);
}

// Logout trainer callback
void on_button_logout_entraineur_clicked(GtkWidget *button, gpointer user_data) {
    current_trainer_id = -1;
    strcpy(current_trainer_id_str, "");
    gtk_widget_hide(window_espace_entraineur);
    gtk_widget_show(window_login);
}

// Logout membre callback
void on_button_logout_membre_clicked(GtkWidget *button, gpointer user_data) {
    current_membre_id = -1;
    strcpy(current_membre_id_str, "");
    gtk_widget_hide(window_espace_membres);
    gtk_widget_show(window_login);
}

// Window show callbacks
void on_window_espace_admin_show(GtkWidget *widget, gpointer user_data) {
    GtkWidget *treeviewequip, *treeviewcour, *combobox_filtre;
    GtkWidget *treeview_centres, *combobox_centre_types;
    GtkWidget *treeview_events;
    GtkWidget *treeview_entraineurs;
    GtkWidget *treeview_membres;
    
    // Refresh equipment treeview
    treeviewequip = lookup_widget(widget, "treeview_listeequipements_admin");
    if (treeviewequip != NULL) {
        refresh_equipment_treeview(treeviewequip);
    }
    
    // Refresh courses treeview
    treeviewcour = lookup_widget(widget, "treeview_liste_cours_admin");
    if (treeviewcour != NULL) {
        refresh_admin_courses_treeview(treeviewcour);
    }
    
    // Refresh equipment types combobox
    combobox_filtre = lookup_widget(widget, "combobox_filter_equip_admin");
    if (combobox_filtre != NULL) {
        refresh_equipment_types_combobox(combobox_filtre, 1);
    }
    
    // NEW: Refresh centres treeview for admin
    treeview_centres = lookup_widget(widget, "treeview_list_centres_admin");
    if (treeview_centres != NULL) {
        refresh_admin_centres_treeview(treeview_centres);
    }
    
    // NEW: Refresh centre types combobox for admin
    combobox_centre_types = lookup_widget(widget, "combobox_filtrer_partype");
    if (combobox_centre_types != NULL) {
        refresh_centre_types_combobox(combobox_centre_types, CENTRE_FILE_PATH);
    }
    //NEW: Refresh events treeview for admin
    
    treeview_events = lookup_widget(widget, "treeview_liste_event_admin");
    if (treeview_events) {
        global_treeview_events = treeview_events;
        afficher_evenements(treeview_events);
        g_print("Events loaded successfully!\n");
    } else {
        g_print("Erreur: TreeView 'treeview_liste_event_admin' introuvable!\n");
    }
    //refresh trainers
    treeview_entraineurs = lookup_widget(widget, "treeview_listentraineurs_admin");
    if (treeview_entraineurs != NULL) {
        afficher_entraineurs(treeview_entraineurs);
    }
    treeview_membres = lookup_widget(widget, "treeview_listemembre_admin");
    if (treeview_membres != NULL) {
        refresh_admin_membres_treeview(treeview_membres);
    }
}

// Update on_window_espace_entraineur_show function:
void on_window_espace_entraineur_show(GtkWidget *widget, gpointer user_data) {
    GtkWidget *treeview, *combobox_type, *combobox_niveau;
    GtkWidget *treeview_centres, *combobox_centre_types;
    GtkWidget *treeviewcour;


    // Refresh courses treeview for trainer
    treeviewcour = lookup_widget(widget, "treeview_cours_dispo_entraineur");
    if (treeviewcour != NULL) {
        refresh_admin_courses_treeview(treeviewcour);
    }
    // Refresh trainer courses treeview
    treeview = lookup_widget(widget, "treeview_liste_mescours_entraineur");
    if (treeview != NULL) {
        refresh_trainer_courses_treeview(treeview, current_trainer_id_str);
    }

    
    // Refresh activity types combobox
    combobox_type = lookup_widget(widget, "combobox_filter_typeactivite_entraineur");
    if (combobox_type != NULL) {
        refresh_activity_types_combobox(combobox_type);
    }
    
    // Refresh niveau combobox
    combobox_niveau = lookup_widget(widget, "combobox_filter_niveau_entraineur");
    if (combobox_niveau != NULL) {
        GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox_niveau));
        if (model != NULL) {
            gtk_list_store_clear(GTK_LIST_STORE(model));
        }
        
        gtk_combo_box_append_text(GTK_COMBO_BOX(combobox_niveau), "Aucune");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combobox_niveau), "Debutant");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combobox_niveau), "Intermediaire");
        gtk_combo_box_append_text(GTK_COMBO_BOX(combobox_niveau), "Avance");
        gtk_combo_box_set_active(GTK_COMBO_BOX(combobox_niveau), 0);
    }
    
    // NEW: Refresh centres treeview for trainer
    treeview_centres = lookup_widget(widget, "treeview_listecentres_entraineur");
    if (treeview_centres != NULL) {
        refresh_trainer_centres_treeview(treeview_centres);
    }
    
    // NEW: Refresh centre types combobox for trainer
    combobox_centre_types = lookup_widget(widget, "combobox_filtrer_partype_entraineur");
    if (combobox_centre_types != NULL) {
        refresh_trainer_centre_types_combobox(combobox_centre_types, CENTRE_FILE_PATH);
    }
   
}


void on_window_espace_membres_show(GtkWidget *widget, gpointer user_data) {
    GtkWidget *treeview_cours;
    GtkWidget *treeview_events;
    GtkWidget *treeview_entraineurs_prives;

    // Load courses treeview (only courses with available places)
    treeview_cours = lookup_widget(widget, "treeview_listes_du_cours");
    if (treeview_cours != NULL) {
        refresh_member_courses_treeview(treeview_cours);
    }
  //NEW: Refresh events treeview for admin
    treeview_events = lookup_widget(widget, "treeview_liste_event_membre");
    if (treeview_events != NULL) {
        refresh_admin_events_treeview(treeview_events);
    }


    
    g_print("DEBUG: Member space window shown\n");
    
    strcpy(current_member_cin, "");
    
    treeview_events = lookup_widget(widget, "treeview_liste_event_membre");
    if (treeview_events) {
        afficher_evenements(treeview_events);
        g_print("Events loaded for member!\n");
    } else {
        g_print("Error: TreeView 'treeview_liste_event_membre' not found!\n");
    }





    treeview_entraineurs_prives = lookup_widget(widget, "treeview_listeentraineur_membre");
    if (treeview_entraineurs_prives != NULL) {
        refresh_membre_entraineurs_prives_treeview(treeview_entraineurs_prives);
    }







}
void on_checkbutton_ajout_priveetrainer_admin_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    if (gtk_toggle_button_get_active(togglebutton)) {
        z = 1;
    } else {
        z = 0;
    }
}

//===========Gestion Centres===========
//   ADMIN
//-------------------------------------
void on_button_ajouter_centre_admin_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *window_ajouter = create_window_ajouter_centre();
    gtk_widget_show(window_ajouter);
}

void on_button_ajout_annuler_admin_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    gtk_widget_hide(window);
}

void on_button_mod_annuler_admin_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    gtk_widget_destroy(window);
}

void on_treeview_list_centres_admin_row_activated(GtkTreeView *treeview,
                                                   GtkTreePath *path,
                                                   GtkTreeViewColumn *column,
                                                   gpointer user_data) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *id;
    
    model = gtk_tree_view_get_model(treeview);
    
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, 0, &id, -1);
        
        // Stocker l'ID du centre sélectionné
        strncpy(centre_selectionne_id, id, 6);
        centre_selectionne_id[6] = '\0';
        
        g_free(id);
        
        // Double-clic : ouvrir directement la fenêtre de modification
        // Chercher le centre
        Centre c = chercher_centre(CENTRE_FILE_PATH, centre_selectionne_id);
        
        if (strcmp(c.id, "-1") == 0) {
            show_message(GTK_WIDGET(treeview), "Centre non trouvé!");
            return;
        }
        
        // Créer et afficher la fenêtre de modification
        GtkWidget *window_modifier = create_window_modifier_centre();
        
    GtkWidget *entry_id = lookup_widget(window_modifier, "entry_mod_idcentre_admin");
    GtkWidget *entry_nom = lookup_widget(window_modifier, "entry_mod_nomcentre_admin");
    GtkWidget *entry_type = lookup_widget(window_modifier, "entry_mod_typecentre_admin");
    GtkWidget *entry_ville = lookup_widget(window_modifier, "entry_mod_ville_centre_admin");
    GtkWidget *spin_cap = lookup_widget(window_modifier, "spinbutton_mod_capacitecentre_admin");
    
    // RadioButtons pour ouvert/fermé
    GtkWidget *radio_ouvert = lookup_widget(window_modifier, "radiobutton_mod_centreouvert_admin");
    GtkWidget *radio_ferme = lookup_widget(window_modifier, "radiobutton_mod_centreferme_admin");
    
    // CheckButtons pour les équipements
    GtkWidget *check_parking = lookup_widget(window_modifier, "checkbutton_mod_parkingcentre_admin");
    GtkWidget *check_wifi = lookup_widget(window_modifier, "checkbutton_mod_wificentre_admin");
    GtkWidget *check_cafeteria = lookup_widget(window_modifier, "checkbutton_mod_cafeteriacentre_admin");
        
        // Remplir les champs
        gtk_entry_set_text(GTK_ENTRY(entry_id), c.id);
        gtk_widget_set_sensitive(entry_id, FALSE);  // ID non modifiable
        gtk_entry_set_text(GTK_ENTRY(entry_nom), c.nom);
        gtk_entry_set_text(GTK_ENTRY(entry_type), c.type);
        gtk_entry_set_text(GTK_ENTRY(entry_ville), c.ville);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_cap), c.capacite);
        
        // Sélectionner le bon radiobutton pour ouvert/fermé
        if (c.ouvert) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_ouvert), TRUE);
        } else {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_ferme), TRUE);
        }
        
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_parking), c.parking);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_wifi), c.wifi);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_cafeteria), c.cafeteria);
        
        gtk_widget_show(window_modifier);
    }
}
void on_spinbutton_cap_min_admin_value_changed(GtkSpinButton *spinbutton, gpointer user_data) {
    filtre_cap_min_actuel = gtk_spin_button_get_value_as_int(spinbutton);
    
    // Appliquer les filtres
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(spinbutton));
    GtkWidget *treeview = lookup_widget(window, "treeview_list_centres_admin");
    if (treeview) {
        appliquer_filtres_et_afficher(treeview);
    }
}

void on_combobox_filtrer_partype_changed(GtkComboBox *combobox, gpointer user_data) {
    gchar *type = gtk_combo_box_get_active_text(combobox);
    
    if (type != NULL) {
        if (strcmp(type, "Tous les types") == 0) {
            strcpy(filtre_type_actuel, "");
        } else {
            strncpy(filtre_type_actuel, type, 99);
            filtre_type_actuel[99] = '\0';
        }
        g_free(type);
        
        // Appliquer les filtres
        GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(combobox));
        GtkWidget *treeview = lookup_widget(window, "treeview_list_centres_admin");
        if (treeview) {
            appliquer_filtres_et_afficher(treeview);
        }
    }
}

void on_radiobutton_tri_ville_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton)) {
        tri_par_nom = 0;
        
        // Appliquer le tri
        GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(togglebutton));
        GtkWidget *treeview = lookup_widget(window, "treeview_list_centres_admin");
        if (treeview) {
            appliquer_filtres_et_afficher(treeview);
        }
    }
}

void on_radiobutton_tri_nom_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    if (gtk_toggle_button_get_active(togglebutton)) {
        tri_par_nom = 1;
        
        // Appliquer le tri
        GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(togglebutton));
        GtkWidget *treeview = lookup_widget(window, "treeview_list_centres_admin");
        if (treeview) {
            appliquer_filtres_et_afficher(treeview);
        }
    }
}

void on_checkbutton_Centres_ouverts_filtrage_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    filtre_centres_ouverts = gtk_toggle_button_get_active(togglebutton);
    
    // Appliquer les filtres
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(togglebutton));
    GtkWidget *treeview = lookup_widget(window, "treeview_list_centres_admin");
    if (treeview) {
        appliquer_filtres_et_afficher(treeview);
    }
}

void on_button_modifier_centre_admin_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    
    // CORRECTION: Get the treeview or selection widget directly
    GtkWidget *treeview_centres = lookup_widget(window, "treeview_list_centres_admin");
    
    if (!treeview_centres) {
        show_message(window, "Erreur: Impossible de trouver la liste des centres!");
        return;
    }
    
    // Get selection from treeview
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_centres));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_message(window, "Veuillez sélectionner un centre dans la liste!");
        return;
    }
    
    // Get the ID from the selected row (assuming ID is in first column)
    char *centre_id = NULL;
    gtk_tree_model_get(model, &iter, 0, &centre_id, -1);
    
    if (!centre_id || strlen(centre_id) == 0) {
        show_message(window, "Centre invalide sélectionné!");
        return;
    }
    
    // Chercher le centre
    Centre c = chercher_centre(CENTRE_FILE_PATH, centre_id);
    
    if (strcmp(c.id, "-1") == 0) {
        show_message(window, "Centre non trouvé!");
        g_free(centre_id);
        return;
    }
    
    // Créer et afficher la fenêtre de modification
    GtkWidget *window_modifier = create_window_modifier_centre();
        
    GtkWidget *entry_id = lookup_widget(window_modifier, "entry_mod_idcentre_admin");
    GtkWidget *entry_nom = lookup_widget(window_modifier, "entry_mod_nomcentre_admin");
    GtkWidget *entry_type = lookup_widget(window_modifier, "entry_mod_typecentre_admin");
    GtkWidget *entry_ville = lookup_widget(window_modifier, "entry_mod_ville_centre_admin");
    GtkWidget *spin_cap = lookup_widget(window_modifier, "spinbutton_mod_capacitecentre_admin");
    
    // RadioButtons pour ouvert/fermé
    GtkWidget *radio_ouvert = lookup_widget(window_modifier, "radiobutton_mod_centreouvert_admin");
    GtkWidget *radio_ferme = lookup_widget(window_modifier, "radiobutton_mod_centreferme_admin");
    
    // CheckButtons pour les équipements
    GtkWidget *check_parking = lookup_widget(window_modifier, "checkbutton_mod_parkingcentre_admin");
    GtkWidget *check_wifi = lookup_widget(window_modifier, "checkbutton_mod_wificentre_admin");
    GtkWidget *check_cafeteria = lookup_widget(window_modifier, "checkbutton_mod_cafeteriacentre_admin");
        
        
    // Remplir les champs avec les bonnes valeurs
    if (entry_id) {
        gtk_entry_set_text(GTK_ENTRY(entry_id), c.id);
        gtk_widget_set_sensitive(entry_id, FALSE);  // ID non modifiable
    }
    
    if (entry_nom) {
        gtk_entry_set_text(GTK_ENTRY(entry_nom), c.nom);
    }
    
    if (entry_type) {
        gtk_entry_set_text(GTK_ENTRY(entry_type), c.type);
    }
    
    if (entry_ville) {
        gtk_entry_set_text(GTK_ENTRY(entry_ville), c.ville);
    }
    
    if (spin_cap) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_cap), c.capacite);
    }
    
    // Sélectionner le bon radiobutton pour ouvert/fermé
    if (radio_ouvert && radio_ferme) {
        if (c.ouvert) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_ouvert), TRUE);
        } else {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_ferme), TRUE);
        }
    }
    
    if (check_parking) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_parking), c.parking);
    }
    
    if (check_wifi) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_wifi), c.wifi);
    }
    
    if (check_cafeteria) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_cafeteria), c.cafeteria);
    }
    
    g_free(centre_id);
    gtk_widget_show(window_modifier);
}
void on_button_supprimer_centre_admin_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    
    // Get the treeview widget directly
    GtkWidget *treeview = lookup_widget(window, "treeview_list_centres_admin");
    
    if (!treeview) {
        show_message(window, "Erreur: Impossible de trouver la liste des centres!");
        return;
    }
    
    // Get selection from treeview
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_message(window, "Veuillez sélectionner un centre dans la liste!");
        return;
    }
    
    // Get the ID from the selected row (assuming ID is in first column)
    char *centre_id = NULL;
    gtk_tree_model_get(model, &iter, 0, &centre_id, -1);
    
    if (!centre_id || strlen(centre_id) == 0) {
        show_message(window, "Centre invalide sélectionné!");
        return;
    }
    
    // Confirmation
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "Êtes-vous sûr de vouloir supprimer le centre ID: %s?",
        centre_id
    );
    
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    if (response == GTK_RESPONSE_YES) {
        if (supprimer_centre(CENTRE_FILE_PATH, centre_id)) {
            show_message(window, "Centre supprimé avec succès!");
            
            // Rafraîchir le treeview
            refresh_admin_centres_treeview(treeview);
        } else {
            show_message(window, "Erreur lors de la suppression du centre!");
        }
    }
    
    // Free the allocated memory
    g_free(centre_id);
}
void on_button_chercher_par_idcentre_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    GtkWidget *entry_id = lookup_widget(window, "entry_recherche_par_idcentre");
    
    const char *id = gtk_entry_get_text(GTK_ENTRY(entry_id));
    
    if (strlen(id) == 0) {
        show_message(window, "Veuillez entrer un ID de centre!");
        return;
    }
    
    // Chercher le centre
    Centre c = chercher_centre(CENTRE_FILE_PATH, id);
    
    if (strcmp(c.id, "-1") == 0) {
        show_message(window, "Centre non trouvé!");
        return;
    }
    
    // Afficher les informations du centre
    char message[1024];
    snprintf(message, sizeof(message),
             "Centre trouvé:\n\n"
             "ID: %s\n"
             "Nom: %s\n"
             "Type: %s\n"
             "Ville: %s\n"
             "Capacité: %d\n"
             "Statut: %s\n"
             "Parking: %s\n"
             "WiFi: %s\n"
             "Cafétéria: %s",
             c.id, c.nom, c.type, c.ville, c.capacite,
             c.ouvert ? "Ouvert" : "Fermé",
             c.parking ? "Oui" : "Non",
             c.wifi ? "Oui" : "Non",
             c.cafeteria ? "Oui" : "Non");
    
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s", message
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    // Sélectionner ce centre
    strncpy(centre_selectionne_id, id, 6);
    centre_selectionne_id[6] = '\0';
}


void on_button_ajout_enregistrer_admin_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    GtkWidget *entry_id, *entry_nom, *entry_type, *entry_ville, *spin_cap;
    GtkWidget *radio_ouvert, *check_parking, *check_wifi, *check_cafeteria;
    
    // Récupérer les widgets
    entry_id = lookup_widget(window, "entry_ajout_idcentre_admin");
    entry_nom = lookup_widget(window, "entry_ajout_nomcentre_admin");
    entry_type = lookup_widget(window, "entry_ajout_type_centre_admin");
    entry_ville = lookup_widget(window, "entry_ajout_villecentre_admin");
    spin_cap = lookup_widget(window, "spinbutton_ajout_capacitecentre_admin");
    
    // RadioButton pour ouvert (si ouvert est activé = 1, sinon = 0)
    radio_ouvert = lookup_widget(window, "radiobutto_ajout_centreouvert_admin");
    
    // CheckButtons pour les équipements
    check_parking = lookup_widget(window, "checkbutton_ajout_parkingcentre_admin");
    check_wifi = lookup_widget(window, "checkbutton_ajout_wificentre_admin");
    check_cafeteria = lookup_widget(window, "checkbutton_ajout_cafeteriacentre_admin");
    
    // Récupérer les valeurs
    const char *id = gtk_entry_get_text(GTK_ENTRY(entry_id));
    const char *nom = gtk_entry_get_text(GTK_ENTRY(entry_nom));
    const char *type = gtk_entry_get_text(GTK_ENTRY(entry_type));
    const char *ville = gtk_entry_get_text(GTK_ENTRY(entry_ville));
    int capacite = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_cap));
    
    // Validation
    if (strlen(id) == 0 || strlen(nom) == 0 || strlen(type) == 0 || strlen(ville) == 0) {
        show_message(window, "Veuillez remplir tous les champs obligatoires!");
        return;
    }
    
    if (capacite <= 0) {
        show_message(window, "La capacité doit être supérieure à 0!");
        return;
    }
    
    // Créer la structure Centre
    Centre c;
    strncpy(c.id, id, 6);
    c.id[6] = '\0';
    strncpy(c.nom, nom, 299);
    c.nom[299] = '\0';
    strncpy(c.type, type, 99);
    c.type[99] = '\0';
    strncpy(c.ville, ville, 199);
    c.ville[199] = '\0';
    c.capacite = capacite;
    
    // Statut ouvert/fermé (1 si radiobutton_ouvert est actif, 0 sinon)
    c.ouvert = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_ouvert)) ? 1 : 0;
    
    c.parking = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_parking));
    c.wifi = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_wifi));
    c.cafeteria = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_cafeteria));
    
    // Ajouter le centre
    if (ajouter_centre(CENTRE_FILE_PATH, c)) {
        show_message(window, "Centre ajouté avec succès!");
        
        // Rafraîchir le treeview dans la fenêtre admin
        GtkWidget *treeview = lookup_widget(window_espace_admin, "treeview_list_centres_admin");
        if (treeview) {
            refresh_admin_centres_treeview(treeview);
        }
        
        // Fermer la fenêtre
        gtk_widget_hide(window);
    } else {
        show_message(window, "Erreur: Ce centre existe déjà ou erreur lors de l'ajout!");
    }
}






void on_button_mod_enrregistrer_admin_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    GtkWidget *entry_id, *entry_nom, *entry_type, *entry_ville, *spin_cap;
    GtkWidget *radio_ouvert, *check_parking, *check_wifi, *check_cafeteria;
    
    // Récupérer les widgets
    entry_id = lookup_widget(window, "entry_mod_idcentre_admin");
    entry_nom = lookup_widget(window, "entry_mod_nomcentre_admin");
    entry_type = lookup_widget(window, "entry_mod_typecentre_admin");
    entry_ville = lookup_widget(window, "entry_mod_ville_centre_admin");
    spin_cap = lookup_widget(window, "spinbutton_mod_capacitecentre_admin");
    
    // RadioButton pour ouvert
    radio_ouvert = lookup_widget(window, "radiobutton_mod_centreouvert_admin");

    // CheckButtons pour les équipements
    check_parking = lookup_widget(window, "checkbutton_mod_parkingcentre_admin");
    check_wifi = lookup_widget(window, "checkbutton_mod_wificentre_admin");
    check_cafeteria = lookup_widget(window, "checkbutton_mod_cafeteriacentre_admin");
    
    // Récupérer les valeurs
    const char *id = gtk_entry_get_text(GTK_ENTRY(entry_id));
    const char *nom = gtk_entry_get_text(GTK_ENTRY(entry_nom));
    const char *type = gtk_entry_get_text(GTK_ENTRY(entry_type));
    const char *ville = gtk_entry_get_text(GTK_ENTRY(entry_ville));
    int capacite = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_cap));
    
    // Validation
    if (strlen(nom) == 0 || strlen(type) == 0 || strlen(ville) == 0) {
        show_message(window, "Veuillez remplir tous les champs obligatoires!");
        return;
    }
    
    if (capacite <= 0) {
        show_message(window, "La capacité doit être supérieure à 0!");
        return;
    }
    
    // Créer la structure Centre
    Centre c;
    strncpy(c.id, id, 6);
    c.id[6] = '\0';
    strncpy(c.nom, nom, 299);
    c.nom[299] = '\0';
    strncpy(c.type, type, 99);
    c.type[99] = '\0';
    strncpy(c.ville, ville, 199);
    c.ville[199] = '\0';
    c.capacite = capacite;
    
    // Statut ouvert/fermé
    c.ouvert = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_ouvert)) ? 1 : 0;
    
    c.parking = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_parking));
    c.wifi = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_wifi));
    c.cafeteria = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_cafeteria));
    
    // Modifier le centre
    if (modifier_centre(CENTRE_FILE_PATH, id, c)) {
        show_message(window, "Centre modifié avec succès!");
        
        // Rafraîchir le treeview
        GtkWidget *treeview = lookup_widget(window_espace_admin, "treeview_list_centres_admin");
        if (treeview) {
            refresh_admin_centres_treeview(treeview);
        }
        
        // Réinitialiser la sélection
        strcpy(centre_selectionne_id, "");
        
        // Fermer la fenêtre
        gtk_widget_destroy(window);
    } else {
        show_message(window, "Erreur lors de la modification du centre!");
    }
}

// ENTRAINEUR
//-------------------------------------------


void
on_treeview_listecentres_entraineur_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}


void
on_spinbutton_cap_min_entraineur_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
filtre_cap_min_actuel = gtk_spin_button_get_value_as_int(spinbutton);
    
    // Appliquer les filtres
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(spinbutton));
    GtkWidget *treeview = lookup_widget(window, "treeview_listecentres_entraineur");
    if (treeview) {
        appliquer_filtres_et_afficher(treeview);
    }

}


void on_button_chercher_centre_entraineur_clicked(GtkButton *button, gpointer user_data) {
   GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    GtkWidget *entry_id = lookup_widget(window, "entry_recherche_parid_centres_entraineur");
    
    const char *id = gtk_entry_get_text(GTK_ENTRY(entry_id));
    
    if (strlen(id) == 0) {
        show_message(window, "Veuillez entrer un ID de centre!");
        return;
    }
    
    // Chercher le centre
    Centre c = chercher_centre(CENTRE_FILE_PATH, id);
    
    if (strcmp(c.id, "-1") == 0) {
        show_message(window, "Centre non trouvé!");
        return;
    }
    
    // Afficher les informations du centre
    char message[1024];
    snprintf(message, sizeof(message),
             "Centre trouvé:\n\n"
             "ID: %s\n"
             "Nom: %s\n"
             "Type: %s\n"
             "Ville: %s\n"
             "Capacité: %d\n"
             "Statut: %s\n"
             "Parking: %s\n"
             "WiFi: %s\n"
             "Cafétéria: %s",
             c.id, c.nom, c.type, c.ville, c.capacite,
             c.ouvert ? "Ouvert" : "Fermé",
             c.parking ? "Oui" : "Non",
             c.wifi ? "Oui" : "Non",
             c.cafeteria ? "Oui" : "Non");
    
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s", message
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    // Sélectionner ce centre
    strncpy(centre_selectionne_id, id, 6);
    centre_selectionne_id[6] = '\0';

}

void
on_combobox_filtrer_partype_entraineur_changed
                                        (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
gchar *type = gtk_combo_box_get_active_text(combobox);
    
    if (type != NULL) {
        if (strcmp(type, "Tous les types") == 0) {
            strcpy(filtre_type_actuel, "");
        } else {
            strncpy(filtre_type_actuel, type, 99);
            filtre_type_actuel[99] = '\0';
        }
        g_free(type);
        
        // Appliquer les filtres
        GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(combobox));
        GtkWidget *treeview = lookup_widget(window, "treeview_listecentres_entraineur");
        if (treeview) {
            appliquer_filtres_et_afficher(treeview);
        }
    }

}


void
on_checkbutton_Centres_ouverts_filtrage_entraineur_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
filtre_centres_ouverts = gtk_toggle_button_get_active(togglebutton);
    
    // Appliquer les filtres
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(togglebutton));
    GtkWidget *treeview = lookup_widget(window, "treeview_listecentres_entraineur");
    if (treeview) {
        appliquer_filtres_et_afficher(treeview);
    }
}


void
on_radiobutton_tri_nom_entraineur_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
if (gtk_toggle_button_get_active(togglebutton)) {
        tri_par_nom = 1;
        
        // Appliquer le tri
        GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(togglebutton));
        GtkWidget *treeview = lookup_widget(window, "treeview_listecentres_entraineur");
        if (treeview) {
            appliquer_filtres_et_afficher(treeview);
        }
    }

}


void
on_radiobutton_tri_ville_entraineur_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
if (gtk_toggle_button_get_active(togglebutton)) {
        tri_par_nom = 0;
        
        // Appliquer le tri
        GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(togglebutton));
        GtkWidget *treeview = lookup_widget(window, "treeview_listecentres_entraineur");
        if (treeview) {
            appliquer_filtres_et_afficher(treeview);
        }
    }

}


void
on_button_sinscrire_entraineur_clicked (GtkButton       *button,
                                        gpointer         user_data)
{


}


void
on_button_voir_inscriptions_entraineur_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_inscri_confirmer_trainer_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_treeview__mesinscriptions_centre_trainer_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}


void
on_checkbutton_mesinscri_selectall_trainer_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


void
on_button_mesincri_centre_modrole_trainer_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mesinscri_centre_annuler_trainer_enter
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mesinscri_retour_trainer_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mesinscri_enregistrermod_trainer_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mesinscri_modannuler_trainer_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}





//====Gestion entraineur(((((chirazzzz))))))
//  Admin

int validate_cin(const char *cin) {
    if (strlen(cin) != 8) return 0;
    for (int i = 0; i < 8; i++) {
        if (cin[i] < '0' || cin[i] > '9') return 0;
    }
    return 1;
}

int validate_name(const char *name) {
    if (strlen(name) < 2) return 0;
    for (int i = 0; name[i] != '\0'; i++) {
        if (!((name[i] >= 'a' && name[i] <= 'z') ||
              (name[i] >= 'A' && name[i] <= 'Z') ||
              name[i] == ' ' || name[i] == '-')) {
            return 0;
        }
    }
    return 1;
}

int validate_date(int jour, int mois, int annee) {
    if (annee < 1950 || annee > 2010) return 0;
    if (mois < 1 || mois > 12) return 0;
    
    int jours_par_mois[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if ((annee % 4 == 0 && annee % 100 != 0) || (annee % 400 == 0)) {
        jours_par_mois[1] = 29;
    }
    
    if (jour < 1 || jour > jours_par_mois[mois - 1]) return 0;
    
    return 1;
}
int validate_date1(int jour, int mois, int annee) {
    if (annee < 1900 || annee > 2100) return 0;
    if (mois < 1 || mois > 12) return 0;
    
    int jours_par_mois[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((annee % 4 == 0 && annee % 100 != 0) || (annee % 400 == 0)) {
        jours_par_mois[1] = 29;
    }
    
    if (jour < 1 || jour > jours_par_mois[mois - 1]) return 0;
    return 1;
}
void
on_treeview_listentraineurs_admin_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}
/******************************************/
void on_treeview_listentraineurs_admin_cursor_changed(GtkTreeView *treeview, gpointer user_data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (selected_cin != NULL) {
        g_free(selected_cin);
        selected_cin = NULL;
    }
    selection = gtk_tree_view_get_selection(treeview);
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, CIN, &selected_cin, -1);
        g_print("Selected CIN: %s\n", selected_cin);
    }
}
/*****************************************/
void on_button_chercher_parid_entraineur_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *entry_recherche;
    GtkWidget *admin_window;
    GtkWidget *treeview;
    const char *recherche;
    int result_count;
    admin_window = lookup_widget(GTK_WIDGET(button), "window_espace_admin");
    entry_recherche = lookup_widget(admin_window, "entry_rechercher_entraineur_parid_admin");
    treeview = lookup_widget(admin_window, "treeview_listentraineurs_admin");
    if (!entry_recherche || !treeview) {
        show_popup_message(admin_window, "Erreur: Widgets introuvables!");
        return;
    }
    recherche = gtk_entry_get_text(GTK_ENTRY(entry_recherche));
    if (strlen(recherche) == 0) {
        afficher_entraineurs(treeview);
        search_mode = 0;
        show_success_message(admin_window, "Liste rafraîchie! Tous les entraîneurs sont affichés.");
        return;
    }
    if (search_mode == 1) {
        afficher_entraineurs(treeview);
        search_mode = 0;
        gtk_entry_set_text(GTK_ENTRY(entry_recherche), "");
        show_success_message(admin_window, "Liste rafraîchie! Tous les entraîneurs sont affichés.");
        return;
    }
    result_count = rechercher_entraineurs(recherche, treeview);
    if (result_count > 0) {
        char message[200];
        search_mode = 1;
        if (result_count == 1) {
            sprintf(message, "Recherche terminée!\n1 entraîneur trouvé.\n\nCliquez à nouveau sur 'Chercher' pour rafraîchir.");
        } else {
            sprintf(message, "Recherche terminée!\n%d entraîneurs trouvés.\n\nCliquez à nouveau sur 'Chercher' pour rafraîchir.", result_count);
        }
        show_success_message(admin_window, message);
    } else {
        show_popup_message(admin_window, "Aucun entraîneur trouvé!\nVeuillez essayer avec un autre critère.");
        search_mode = 0;
    }
}


void on_button_ajouter_entraineur_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *window_ajouter;
    GtkWidget *admin_window;
    w = 1;
    z = 1;
    is_modify_mode = 0;
    admin_window = lookup_widget(GTK_WIDGET(button), "window_espace_admin");
    if (admin_window) {
        global_treeview_entraineurs = lookup_widget(admin_window, "treeview_listentraineurs_admin");
    }
    window_ajouter = create_window_ajouter_entraineur();
    gtk_widget_show(window_ajouter);
}


/******************* MODIFIER BUTTON - Opens modify window with data ********************/
void on_button_modifier_entraineur_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *admin_window;
    GtkWidget *window_modifier;
    entraineur e;
    admin_window = lookup_widget(GTK_WIDGET(button), "window_espace_admin");
    if (selected_cin == NULL || strlen(selected_cin) == 0) {
        show_popup_message(admin_window, "Veuillez sélectionner un entraîneur à modifier!");
        return;
    }
    e = trouver_entraineur(selected_cin);
    if (strlen(e.cin_e) == 0) {
        show_popup_message(admin_window, "Erreur: Entraîneur introuvable!");
        return;
    }
    is_modify_mode = 1;
    global_treeview_entraineurs = lookup_widget(admin_window, "treeview_listentraineurs_admin");
    window_modifier = create_window_modifier_entraineur();
    GtkWidget *entry_cin = lookup_widget(window_modifier, "entry_mod_cintrainer_admin");
    GtkWidget *entry_nom = lookup_widget(window_modifier, "entry_mod_nomtrainer_admin");
    GtkWidget *entry_prenom = lookup_widget(window_modifier, "entry_mod_prenomtrainer_admin");
    GtkWidget *spinbutton_jour = lookup_widget(window_modifier, "spinbutton_mod_jourtrainer_admin");
    GtkWidget *spinbutton_mois = lookup_widget(window_modifier, "spinbutton_mod_moistrainer_admin");
    GtkWidget *spinbutton_annee = lookup_widget(window_modifier, "spinbutton_mod_anneetrainer_admin");
    GtkWidget *combobox_specialite = lookup_widget(window_modifier, "combobox_mod_specialittetrainer_admin");
    GtkWidget *radiobutton_homme = lookup_widget(window_modifier, "radiobutton_mod_hommetrainer_admin");
    GtkWidget *radiobutton_femme = lookup_widget(window_modifier, "radiobutton_mod_femmetrainer_admin");
    GtkWidget *checkbutton_prive = lookup_widget(window_modifier, "checkbutton_mod_priveetrainer_admin");
    gtk_entry_set_text(GTK_ENTRY(entry_cin), e.cin_e);
    gtk_widget_set_sensitive(entry_cin, FALSE);
    gtk_entry_set_text(GTK_ENTRY(entry_nom), e.nom_e);
    gtk_entry_set_text(GTK_ENTRY(entry_prenom), e.prenom_e);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton_jour), e.date_nai_e.j);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton_mois), e.date_nai_e.m);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton_annee), e.date_nai_e.a);
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combobox_specialite))), e.specialite_e);
    if (strcmp(e.sexe_e, "Homme") == 0) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton_homme), TRUE);
        w = 1;
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton_femme), TRUE);
        w = 0;
    }
    if (strcmp(e.entrainement_prive, "1") == 0) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_prive), TRUE);
        z = 1;
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_prive), FALSE);
        z = 0;
    }
    gtk_widget_show(window_modifier);
}


void on_button_supprimer_entraineur_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *admin_window;
    GtkWidget *treeview;
    GtkWidget *dialog;
    gint response;
    admin_window = lookup_widget(GTK_WIDGET(button), "window_espace_admin");
    treeview = lookup_widget(admin_window, "treeview_listentraineurs_admin");
    if (selected_cin == NULL || strlen(selected_cin) == 0) {
        show_popup_message(admin_window, "Veuillez sélectionner un entraîneur à supprimer!");
        return;
    }
    dialog = gtk_message_dialog_new(GTK_WINDOW(admin_window),
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_YES_NO,
                                   "Êtes-vous sûr de vouloir supprimer l'entraîneur avec CIN: %s?",
                                   selected_cin);
    gtk_window_set_title(GTK_WINDOW(dialog), "Confirmation de suppression");
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (response == GTK_RESPONSE_YES) {
        supprimer_entraineur(selected_cin);
        show_success_message(admin_window, "Entraîneur supprimé avec succès!");
        if (treeview) {
            afficher_entraineurs(treeview);
            search_mode = 0;
        }
        if (selected_cin) {
            g_free(selected_cin);
            selected_cin = NULL;
        }
    }
}


void
on_treeview_listemembre_admin_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}


void
on_button_ajouter_membre_admmin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_modifier_membre_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_supprimer_membre_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_chercher_membre_parid_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_treeview_listeequipements_admin_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}


void
on_button_chercher_equipid_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_ajouter_equip_admin_clicked  (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_modifier_equip_admin_clicked (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_supprimer_equip_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_treeview_liste_cours_admin_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}


void
on_button_chercher_parid_cours_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_ajouter_cours_sportifs_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_modifier_cours_admin_clicked (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_supprimer_cours_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


/************************************************************************************************************wejdennnnnnnnnn/
/*************************************************************************/

void populate_type_combobox(GtkWidget *combobox) {
    // Clear existing items
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
    if (model) {
        gtk_list_store_clear(GTK_LIST_STORE(model));
    }
    
    // Add event types
    gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Course");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Yoga");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Boxe");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Natation");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Fitness");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Musculation");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Cyclisme");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Danse");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Arts_Martiaux");
    
    // Set first item as default
    gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
}

void populate_centre_combobox(GtkWidget *combobox) {
    Centre centres[MAX_CENTRES];
    int nb_centres = 0;
    
    // Load centres from file
    if (charger_centres("centres.txt", centres, &nb_centres) != 0) {
        g_print("Error: Could not load centres\n");
        return;
    }
    
    // Clear existing items
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
    if (model) {
        gtk_list_store_clear(GTK_LIST_STORE(model));
    }
    
    // Add centres to combobox (only open centres)
    for (int i = 0; i < nb_centres; i++) {
        if (centres[i].ouvert) {
            gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), centres[i].nom);
        }
    }
    
    // Set first item as default
    if (nb_centres > 0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
    }
    
    g_print("DEBUG: Loaded %d centres into combobox\n", nb_centres);
}


/******************* Helper Functions for Popups *******************/

void show_popup_message(GtkWidget *parent, const char *message) {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_WARNING,
                                   GTK_BUTTONS_OK,
                                   "%s", message);
    gtk_window_set_title(GTK_WINDOW(dialog), "Attention");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void show_success_message(GtkWidget *parent, const char *message) {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK,
                                   "%s", message);
    gtk_window_set_title(GTK_WINDOW(dialog), "Succès");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
void on_treeview_liste_event_admin_cursor_changed(GtkTreeView *treeview, gpointer user_data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    selected_event_id = 0;
    selection = gtk_tree_view_get_selection(treeview);
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, ID_EVENT_COL, &selected_event_id, -1);
        g_print("Selected Event ID: %d\n", selected_event_id);
    }
}
void
on_treeview_liste_event_admin_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}

/*****************************************************/
void on_button_chercher_parid_event_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *entry_recherche;
    GtkWidget *admin_window;
    GtkWidget *treeview;
    const char *recherche;
    int result_count;
    
    admin_window = lookup_widget(GTK_WIDGET(button), "window_espace_admin");
    entry_recherche = lookup_widget(admin_window, "entry_chercher_parid_event_admin");
    treeview = lookup_widget(admin_window, "treeview_liste_event_admin");
    
    if (!entry_recherche || !treeview) {
        show_popup_message(admin_window, "Erreur: Widgets introuvables!");
        return;
    }
    
    recherche = gtk_entry_get_text(GTK_ENTRY(entry_recherche));
    
    if (strlen(recherche) == 0) {
        afficher_evenements(treeview);
        event_search_mode = 0;
        show_success_message(admin_window, "Liste rafraîchie! Tous les événements sont affichés.");
        return;
    }
    
    if (event_search_mode == 1) {
        afficher_evenements(treeview);
        event_search_mode = 0;
        gtk_entry_set_text(GTK_ENTRY(entry_recherche), "");
        show_success_message(admin_window, "Liste rafraîchie! Tous les événements sont affichés.");
        return;
    }
    
    result_count = rechercher_evenements(recherche, treeview);
    
    if (result_count > 0) {
        char message[200];
        event_search_mode = 1;
        
        if (result_count == 1) {
            sprintf(message, "Recherche terminée!\n1 événement trouvé.\n\nCliquez à nouveau sur 'Chercher' pour rafraîchir.");
        } else {
            sprintf(message, "Recherche terminée!\n%d événements trouvés.\n\nCliquez à nouveau sur 'Chercher' pour rafraîchir.", result_count);
        }
        show_success_message(admin_window, message);
    } else {
        show_popup_message(admin_window, "Aucun événement trouvé!\nVeuillez essayer avec un autre critère.");
        event_search_mode = 0;
    }
}


void on_button_ajouter_event_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *window_ajouter;
    GtkWidget *admin_window;
    
    status_event = 0;
    
    admin_window = lookup_widget(GTK_WIDGET(button), "window_espace_admin");
    if (admin_window) {
        global_treeview_events = lookup_widget(admin_window, "treeview_liste_event_admin");
    }
    
    window_ajouter = create_window_ajouter_event();
    gtk_widget_show(window_ajouter);
}


void on_button_modifier_event_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *admin_window;
    GtkWidget *window_modifier;
    evenement ev;
    
    admin_window = lookup_widget(GTK_WIDGET(button), "window_espace_admin");
    
    if (selected_event_id == 0) {
        show_popup_message(admin_window, "Veuillez sélectionner un événement à modifier!");
        return;
    }
    
    ev = trouver_evenement(selected_event_id);
    
    if (ev.id_event == 0) {
        show_popup_message(admin_window, "Erreur: Événement introuvable!");
        return;
    }
    
    global_treeview_events = lookup_widget(admin_window, "treeview_liste_event_admin");
    
    window_modifier = create_window_modifier_event();
    
    GtkWidget *entry_nom = lookup_widget(window_modifier, "entry_mod_nomevent_admin");
    GtkWidget *spinbutton_jour = lookup_widget(window_modifier, "spinbutton_mod_jourevent_admin");
    GtkWidget *spinbutton_mois = lookup_widget(window_modifier, "spinbutton_mod_moisevent_admin");
    GtkWidget *spinbutton_annee = lookup_widget(window_modifier, "spinbutton_mod_anneeevent_admin");
    GtkWidget *combobox_type = lookup_widget(window_modifier, "combobox_mod_typeevent_admin");
    GtkWidget *combobox_centre = lookup_widget(window_modifier, "combobox_mod_centreevent_admin");
    GtkWidget *radiobutton_payant = lookup_widget(window_modifier, "radiobutton_mod_payant");
    GtkWidget *radiobutton_gratuit = lookup_widget(window_modifier, "radiobutton_mod_gratuit");
    
    gtk_entry_set_text(GTK_ENTRY(entry_nom), ev.nom_event);
    
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton_jour), ev.date_e.j);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton_mois), ev.date_e.m);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton_annee), ev.date_e.a);
    
    // Set Type in combobox by finding the matching item
    if (combobox_type) {
        GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox_type));
        GtkTreeIter iter;
        gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
        int index = 0;
        
        while (valid) {
            gchar *str_data;
            gtk_tree_model_get(model, &iter, 0, &str_data, -1);
            
            if (strcmp(str_data, ev.type_event) == 0) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(combobox_type), index);
                g_free(str_data);
                break;
            }
            
            g_free(str_data);
            valid = gtk_tree_model_iter_next(model, &iter);
            index++;
        }
    }
    
    // Set Centre in combobox by finding the matching item
    if (combobox_centre) {
        GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox_centre));
        GtkTreeIter iter;
        gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
        int index = 0;
        
        while (valid) {
            gchar *str_data;
            gtk_tree_model_get(model, &iter, 0, &str_data, -1);
            
            if (strcmp(str_data, ev.centre) == 0) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(combobox_centre), index);
                g_free(str_data);
                break;
            }
            
            g_free(str_data);
            valid = gtk_tree_model_iter_next(model, &iter);
            index++;
        }
    }
    
    // Set status radio buttons
    if (strcmp(ev.statut, "payant") == 0) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton_payant), TRUE);
        status_event = 1;
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton_gratuit), TRUE);
        status_event = 0;
    }
    
    gtk_widget_show(window_modifier);
}

void on_button_supprimer_event_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *admin_window;
    GtkWidget *treeview;
    GtkWidget *dialog;
    gint response;
    
    admin_window = lookup_widget(GTK_WIDGET(button), "window_espace_admin");
    treeview = lookup_widget(admin_window, "treeview_liste_event_admin");
    
    if (selected_event_id == 0) {
        show_popup_message(admin_window, "Veuillez sélectionner un événement à supprimer!");
        return;
    }
    
    dialog = gtk_message_dialog_new(GTK_WINDOW(admin_window),
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_YES_NO,
                                   "Êtes-vous sûr de vouloir supprimer l'événement ID: %d?",
                                   selected_event_id);
    
    gtk_window_set_title(GTK_WINDOW(dialog), "Confirmation de suppression");
    
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    if (response == GTK_RESPONSE_YES) {
        supprimer_evenement(selected_event_id);
        show_success_message(admin_window, "Événement supprimé avec succès!");
        
        if (treeview) {
            afficher_evenements(treeview);
            event_search_mode = 0;
        }
        
        selected_event_id = 0;
    }
}
/******************* Date Validation Function *******************/


/******************* Helper Functions ********************/

int validate_event_name(const char *name) {
    if (strlen(name) < 3) return 0;
    return 1;
}


void on_button_ajout_enregistrerevent_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *window_ajouter;
    GtkWidget *entry_nom;
    GtkWidget *spinbutton_jour, *spinbutton_mois, *spinbutton_annee;
    GtkWidget *combobox_type, *combobox_centre;
    
    evenement ev;
    const char *nom, *type, *centre;
    int jour, mois, annee;
    
    window_ajouter = lookup_widget(GTK_WIDGET(button), "window_ajouter_event");
    
    entry_nom = lookup_widget(window_ajouter, "entry_ajout_nomevent_admin");
    spinbutton_jour = lookup_widget(window_ajouter, "spinbutton_ajout_jourevent_admin");
    spinbutton_mois = lookup_widget(window_ajouter, "spinbutton_ajout_moisevent_admin");
    spinbutton_annee = lookup_widget(window_ajouter, "spinbutton_ajout_anneeevent_admin");
    combobox_type = lookup_widget(window_ajouter, "combobox_ajout_typeevent_admin");
    combobox_centre = lookup_widget(window_ajouter, "combobox_ajout_eventcentre_admin");
    
    nom = gtk_entry_get_text(GTK_ENTRY(entry_nom));
    jour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton_jour));
    mois = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton_mois));
    annee = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton_annee));
    
    type = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combobox_type));
    centre = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combobox_centre));
    
    if (strlen(nom) == 0) {
        show_popup_message(window_ajouter, "Erreur: Le nom de l'événement est obligatoire!");
        return;
    }
    
    if (!validate_event_name(nom)) {
        show_popup_message(window_ajouter, "Erreur: Le nom doit contenir au moins 3 caractères!");
        return;
    }
    
    if (type == NULL || strlen(type) == 0) {
        show_popup_message(window_ajouter, "Erreur: Veuillez sélectionner un type d'événement!");
        return;
    }
    
    if (centre == NULL || strlen(centre) == 0) {
        show_popup_message(window_ajouter, "Erreur: Veuillez sélectionner un centre!");
        return;
    }
    
    if (!validate_date1(jour, mois, annee)) {
        show_popup_message(window_ajouter, "Erreur: Date d'événement invalide!");
        return;
    }
    
    strcpy(ev.nom_event, nom);
    strcpy(ev.type_event, type);
    strcpy(ev.centre, centre);
    
    ev.date_e.j = jour;
    ev.date_e.m = mois;
    ev.date_e.a = annee;
    
    // Set status based on payant/gratuit
    if (status_event == 1) {
        strcpy(ev.statut, "payant");
    } else {
        strcpy(ev.statut, "gratuit");
    }
    
    g_print("DEBUG: Before saving - status_event=%d, statut=%s\n", status_event, ev.statut);
    
    ajouter_evenement(ev);
    show_success_message(window_ajouter, "Événement ajouté avec succès!");
    
    if (global_treeview_events != NULL) {
        afficher_evenements(global_treeview_events);
        event_search_mode = 0;
    }
    
    gtk_widget_destroy(window_ajouter);
}



void on_button_ajout_annulerevent_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *window_ajouter;
    window_ajouter = lookup_widget(GTK_WIDGET(button), "window_ajouter_event");
    gtk_widget_destroy(window_ajouter);
}


void on_button_mod_enregistrerevent_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *window_modifier;
    GtkWidget *entry_nom;
    GtkWidget *spinbutton_jour, *spinbutton_mois, *spinbutton_annee;
    GtkWidget *combobox_type, *combobox_centre;
    
    evenement ev;
    const char *nom, *type, *centre;
    int jour, mois, annee;
    
    window_modifier = lookup_widget(GTK_WIDGET(button), "window_modifier_event");
    
    entry_nom = lookup_widget(window_modifier, "entry_mod_nomevent_admin");
    spinbutton_jour = lookup_widget(window_modifier, "spinbutton_mod_jourevent_admin");
    spinbutton_mois = lookup_widget(window_modifier, "spinbutton_mod_moisevent_admin");
    spinbutton_annee = lookup_widget(window_modifier, "spinbutton_mod_anneeevent_admin");
    combobox_type = lookup_widget(window_modifier, "combobox_mod_typeevent_admin");
    combobox_centre = lookup_widget(window_modifier, "combobox_mod_centreevent_admin");
    
    nom = gtk_entry_get_text(GTK_ENTRY(entry_nom));
    jour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton_jour));
    mois = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton_mois));
    annee = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton_annee));
    
    type = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combobox_type));
    centre = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combobox_centre));
    
    if (strlen(nom) == 0) {
        show_popup_message(window_modifier, "Erreur: Le nom de l'événement est obligatoire!");
        return;
    }
    
    if (!validate_event_name(nom)) {
        show_popup_message(window_modifier, "Erreur: Le nom doit contenir au moins 3 caractères!");
        return;
    }
    
    if (type == NULL || strlen(type) == 0) {
        show_popup_message(window_modifier, "Erreur: Veuillez sélectionner un type d'événement!");
        return;
    }
    
    if (centre == NULL || strlen(centre) == 0) {
        show_popup_message(window_modifier, "Erreur: Veuillez sélectionner un centre!");
        return;
    }
    
    if (!validate_date1(jour, mois, annee)) {
        show_popup_message(window_modifier, "Erreur: Date d'événement invalide!");
        return;
    }
    
    ev.id_event = selected_event_id;
    strcpy(ev.nom_event, nom);
    strcpy(ev.type_event, type);
    strcpy(ev.centre, centre);
    
    ev.date_e.j = jour;
    ev.date_e.m = mois;
    ev.date_e.a = annee;
    
    // Set status based on payant/gratuit
    if (status_event == 1) {
        strcpy(ev.statut, "payant");
    } else {
        strcpy(ev.statut, "gratuit");
    }
    
    g_print("DEBUG: Before saving - status_event=%d, statut=%s\n", status_event, ev.statut);
    
    modifier_evenement(ev);
    show_success_message(window_modifier, "Événement modifié avec succès!");
    
    if (global_treeview_events != NULL) {
        afficher_evenements(global_treeview_events);
        event_search_mode = 0;
    }
    
    gtk_widget_destroy(window_modifier);
}



void on_button_mod_annulerevent_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *window_modifier;
    window_modifier = lookup_widget(GTK_WIDGET(button), "window_modifier_event");
    gtk_widget_destroy(window_modifier);
}
void on_radiobutton_mod_payant__toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    if (gtk_toggle_button_get_active(togglebutton)) {
        status_event = 1;
        g_print("DEBUG: Status set to 'payant' (1)\n");
    }
}

void on_radiobutton_mod_gratuit_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    if (gtk_toggle_button_get_active(togglebutton)) {
        status_event = 0;
        g_print("DEBUG: Status set to 'gratuit' (0)\n");
    }
}


void on_window_ajouter_event_show(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *combobox_type;
    GtkWidget *combobox_centre;
    
    g_print("DEBUG: window_ajouter_event_show called\n");
    
    // Get the comboboxes
    combobox_type = lookup_widget(widget, "combobox_ajout_typeevent_admin");
    combobox_centre = lookup_widget(widget, "combobox_ajout_eventcentre_admin");
    
    // Populate them
    if (combobox_type) {
        populate_type_combobox(combobox_type);
        g_print("DEBUG: Type combobox populated\n");
    } else {
        g_print("ERROR: Type combobox not found\n");
    }
    
    if (combobox_centre) {
        populate_centre_combobox(combobox_centre);
        g_print("DEBUG: Centre combobox populated\n");
    } else {
        g_print("ERROR: Centre combobox not found\n");
    }
}

void on_window_modifier_event_show(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *combobox_type;
    GtkWidget *combobox_centre;
    
    g_print("DEBUG: window_modifier_event_show called\n");
    
    // Get the comboboxes
    combobox_type = lookup_widget(widget, "combobox_mod_typeevent_admin");
    combobox_centre = lookup_widget(widget, "combobox_mod_centreevent_admin");
    
    // Populate them
    if (combobox_type) {
        populate_type_combobox(combobox_type);
        g_print("DEBUG: Type combobox populated\n");
    }
    
    if (combobox_centre) {
        populate_centre_combobox(combobox_centre);
        g_print("DEBUG: Centre combobox populated\n");
    }
}

/******************* ADD FORM - Radio Buttons ********************/

void on_radiobutton_mod_payant_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    if (gtk_toggle_button_get_active(togglebutton)) {
        status_event = 1;
        g_print("DEBUG: Status set to 'payant' (1)\n");
    }
}
void on_radiobutton_payant_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    if (gtk_toggle_button_get_active(togglebutton)) {
        status_event = 1;
        g_print("DEBUG: Status set to 'payant' (1)\n");
    }
}

void on_radiobutton_gratuit_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    if (gtk_toggle_button_get_active(togglebutton)) {
        status_event = 0;
        g_print("DEBUG: Status set to 'gratuit' (0)\n");
    }
}
void on_button_supp_myevent_membre_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *mesevent_window;
    GtkWidget *treeview;
    GtkWidget *dialog;
    gint response;
    
    mesevent_window = lookup_widget(GTK_WIDGET(button), "window_mesevent_membre");
    treeview = lookup_widget(mesevent_window, "treeview_listevent_membre");
    
    if (selected_inscription_id == 0) {
        show_popup_message(mesevent_window, "Veuillez sélectionner une inscription à supprimer!");
        return;
    }
    
    dialog = gtk_message_dialog_new(GTK_WINDOW(mesevent_window),
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_YES_NO,
                                   "Êtes-vous sûr de vouloir annuler cette inscription?");
    
    gtk_window_set_title(GTK_WINDOW(dialog), "Confirmation de suppression");
    
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    if (response == GTK_RESPONSE_YES) {
        supprimer_inscription_event(selected_inscription_id);
        show_success_message(mesevent_window, "Inscription annulée avec succès!");
        
        if (treeview && strlen(current_member_cin) > 0) {
            afficher_inscriptions_membre(treeview, current_member_cin);
            my_event_search_mode = 0;
        }
        
        selected_inscription_id = 0;
    }
}


void on_button_mesevent_retour_membre_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *window_mesevent;
    window_mesevent = lookup_widget(GTK_WIDGET(button), "window_mesevent_membre");
    gtk_widget_destroy(window_mesevent);
}
void on_treeview_liste_event_membre_cursor_changed(GtkTreeView *treeview, gpointer user_data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    selected_event_id_membre = 0;
    selection = gtk_tree_view_get_selection(treeview);
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, ID_EVENT_COL, &selected_event_id_membre, -1);
        g_print("Selected Event ID (Member): %d\n", selected_event_id_membre);
    }
}


void on_treeview_listevent_membre_cursor_changed(GtkTreeView *treeview, gpointer user_data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    selected_inscription_id = 0;
    selection = gtk_tree_view_get_selection(treeview);
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, ID_INSC_COL, &selected_inscription_id, -1);
        g_print("Selected Inscription ID: %d\n", selected_inscription_id);
    }
}


void on_window_mesevent_membre_show(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *treeview_inscriptions;
    
    g_print("DEBUG: My events window shown for CIN: %s\n", current_member_cin);
    
    treeview_inscriptions = lookup_widget(widget, "treeview_listevent_membre");
    
    if (!treeview_inscriptions) {
        g_print("ERROR: TreeView 'treeview_listevent_membre' not found in window_mesevent_membre!\n");
        return;
    }
    
    if (strlen(current_member_cin) == 0) {
        g_print("ERROR: CIN is empty!\n");
        show_popup_message(widget, "Erreur: CIN introuvable!");
        return;
    }
    
    g_print("DEBUG: Loading enrollments for CIN: %s\n", current_member_cin);
    afficher_inscriptions_membre(treeview_inscriptions, current_member_cin);
    g_print("DEBUG: Enrollments loaded successfully!\n");
}

void on_button_chercher_event_membre_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *entry_recherche;
    GtkWidget *mesevent_window;
    GtkWidget *treeview;
    const char *recherche;
    int result_count;
    
    mesevent_window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    if (!GTK_IS_WINDOW(mesevent_window)) {
        g_print("ERROR: Could not get parent window!\n");
        return;
    }
    
    g_print("DEBUG: Search button clicked in window_mesevent_membre\n");
    
    entry_recherche = lookup_widget(mesevent_window, "entry_rechercheid_event_membre");
    treeview = lookup_widget(mesevent_window, "treeview_liste_event_membre");
    
    if (!entry_recherche || !treeview) {
        show_popup_message(mesevent_window, "Erreur: Widgets introuvables!");
        return;
    }
    
    if (strlen(current_member_cin) == 0) {
        show_popup_message(mesevent_window, "Erreur: CIN du membre introuvable!");
        return;
    }
    
    recherche = gtk_entry_get_text(GTK_ENTRY(entry_recherche));
    
    if (strlen(recherche) == 0) {
        afficher_inscriptions_membre(treeview, current_member_cin);
        my_event_search_mode = 0;
        show_success_message(mesevent_window, "Liste rafraîchie!");
        return;
    }
    
    if (my_event_search_mode == 1) {
        afficher_inscriptions_membre(treeview, current_member_cin);
        my_event_search_mode = 0;
        gtk_entry_set_text(GTK_ENTRY(entry_recherche), "");
        show_success_message(mesevent_window, "Liste rafraîchie!");
        return;
    }
    
    result_count = rechercher_mes_inscriptions(recherche, treeview, current_member_cin);
    
    if (result_count > 0) {
        char message[200];
        my_event_search_mode = 1;
        
        if (result_count == 1) {
            sprintf(message, "Recherche terminée!\n1 inscription trouvée.\n\nCliquez à nouveau sur 'Chercher' pour rafraîchir.");
        } else {
            sprintf(message, "Recherche terminée!\n%d inscriptions trouvées.\n\nCliquez à nouveau sur 'Chercher' pour rafraîchir.", result_count);
        }
        show_success_message(mesevent_window, message);
    } else {
        show_popup_message(mesevent_window, "Aucune inscription trouvée!\nVeuillez essayer avec un autre critère.");
        my_event_search_mode = 0;
    }
}


void on_button_sinscrire_event_membre_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *membre_window;
    GtkWidget *entry_cin;
    inscription_event insc;
    evenement ev;
    const char *cin_text;
    
    membre_window = lookup_widget(GTK_WIDGET(button), "window_espace_membres");
    
    entry_cin = lookup_widget(membre_window, "entry_cin_membre_event");
    if (!entry_cin) {
        show_popup_message(membre_window, "Erreur: Champ CIN introuvable!");
        return;
    }
    
    cin_text = gtk_entry_get_text(GTK_ENTRY(entry_cin));
    
    if (strlen(cin_text) == 0) {
        show_popup_message(membre_window, "Veuillez entrer votre CIN avant de vous inscrire!");
        return;
    }
    
    if (strlen(cin_text) < 8) {
        show_popup_message(membre_window, "Le CIN doit contenir au moins 8 caractères!");
        return;
    }
    
    strcpy(current_member_cin, cin_text);
    g_print("DEBUG: CIN entered: %s\n", current_member_cin);
    
    if (selected_event_id_membre == 0) {
        show_popup_message(membre_window, "Veuillez sélectionner un événement pour vous inscrire!");
        return;
    }
    
    if (inscription_existe(current_member_cin, selected_event_id_membre)) {
        show_popup_message(membre_window, "Vous êtes déjà inscrit à cet événement!");
        return;
    }
    
    ev = trouver_evenement(selected_event_id_membre);
    
    if (ev.id_event == 0) {
        show_popup_message(membre_window, "Erreur: Événement introuvable!");
        return;
    }
    
    strcpy(insc.cin_membre, current_member_cin);
    insc.id_event = ev.id_event;
    strcpy(insc.nom_event, ev.nom_event);
    strcpy(insc.type_event, ev.type_event);
    strcpy(insc.centre, ev.centre);
    insc.date_e.j = ev.date_e.j;
    insc.date_e.m = ev.date_e.m;
    insc.date_e.a = ev.date_e.a;
    
    ajouter_inscription_event(insc);
    
    char success_msg[300];
    sprintf(success_msg, "Inscription réussie!\n\nVous êtes maintenant inscrit à:\n%s\nStatut: %s\n\nCIN: %s", 
            ev.nom_event, ev.statut, current_member_cin);
    show_success_message(membre_window, success_msg);
    
    g_print("DEBUG: Member %s enrolled in event %d\n", current_member_cin, selected_event_id_membre);
}

void on_button_mesinscriptions_membre_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *membre_window;
    GtkWidget *entry_cin;
    GtkWidget *window_mesevent;
    const char *cin_text;
    
    membre_window = lookup_widget(GTK_WIDGET(button), "window_espace_membres");
    
    entry_cin = lookup_widget(membre_window, "entry_cin_membre_event");
    if (!entry_cin) {
        show_popup_message(membre_window, "Erreur: Champ CIN introuvable!");
        return;
    }
    
    cin_text = gtk_entry_get_text(GTK_ENTRY(entry_cin));
    
    if (strlen(cin_text) == 0) {
        show_popup_message(membre_window, "Veuillez entrer votre CIN pour voir vos inscriptions!");
        return;
    }
    
    if (strlen(cin_text) < 8) {
        show_popup_message(membre_window, "Le CIN doit contenir au moins 8 caractères!");
        return;
    }
    
    strcpy(current_member_cin, cin_text);
    g_print("DEBUG: CIN for my events: %s\n", current_member_cin);
    
    window_mesevent = create_window_mesevent_membre();
    gtk_widget_show(window_mesevent);
}

/****************************************************************************************/
void
on_radiobutton_filtre_payant_event_admin_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


void
on_radiobutton_filtre_gratuit_event_admin_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}








void
on_button_chercher_mescoursid_entraineur_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_reserver_equip_entraineur_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_equip_reserve_entraineur_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_treeview_cours_dispo_entraineur_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}




void
on_treeview_listes_du_cours__row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}


void
on_button_chercher_cours_membre_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_radiobutton_debutant_cours_membre_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


void
on_radiobutton_intermidiaire_cours_membre_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


void
on_button_sinscrirecours_membre_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mescours_membre_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_radiobutton_avance_cours_membre_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


void
on_treeview_listeentraineur_membre_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}


void
on_button_chercher_entraineur_membre_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_demander_entraineur_membre_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mesentraineurs_membre_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_treeview_liste_event_membre_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}



void
on_radiobutton_filtre_gratuit_membre_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


void
on_radiobutton_filtre_payant_event_membre_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}





/******************* ADD FORM - ENREGISTRER BUTTON ********************/
void on_button_ajout_enregistrertrainer_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *window_ajouter;
    GtkWidget *entry_cin, *entry_nom, *entry_prenom;
    GtkWidget *spinbutton_jour, *spinbutton_mois, *spinbutton_annee;
    GtkWidget *combobox_specialite;
    entraineur e;
    const char *cin, *nom, *prenom, *specialite;
    int jour, mois, annee;
    window_ajouter = lookup_widget(GTK_WIDGET(button), "window_ajouter_entraineur");
    entry_cin = lookup_widget(window_ajouter, "entry_ajout_cintrainer_admin");
    entry_nom = lookup_widget(window_ajouter, "entry_ajout_nomtrainner_admin");
    entry_prenom = lookup_widget(window_ajouter, "entry_ajout_prenomtrainer_admin");
    spinbutton_jour = lookup_widget(window_ajouter, "spinbutton_ajout__jourtrainer_admin");
    spinbutton_mois = lookup_widget(window_ajouter, "spinbutton_ajout_moistrainer_admin");
    spinbutton_annee = lookup_widget(window_ajouter, "spinbutton_ajout_anneetrainer_admin");
    combobox_specialite = lookup_widget(window_ajouter, "combobox_ajout_specialittetrainer_admin");
    cin = gtk_entry_get_text(GTK_ENTRY(entry_cin));
    nom = gtk_entry_get_text(GTK_ENTRY(entry_nom));
    prenom = gtk_entry_get_text(GTK_ENTRY(entry_prenom));
    jour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton_jour));
    mois = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton_mois));
    annee = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton_annee));
    specialite = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combobox_specialite));
    if (strlen(cin) == 0) {
        show_popup_message(window_ajouter, "Erreur: Le CIN est obligatoire!");
        return;
    }
    if (!validate_cin(cin)) {
        show_popup_message(window_ajouter, "Erreur: Le CIN doit contenir exactement 8 chiffres!");
        return;
    }
    if (entraineur_existe(cin)) {
        show_popup_message(window_ajouter, "Erreur: Un entraîneur avec ce CIN existe déjà!");
        return;
    }
    if (strlen(nom) == 0) {
        show_popup_message(window_ajouter, "Erreur: Le nom est obligatoire!");
        return;
    }
    if (!validate_name(nom)) {
        show_popup_message(window_ajouter, "Erreur: Le nom doit contenir uniquement des lettres!");
        return;
    }
    if (strlen(prenom) == 0) {
        show_popup_message(window_ajouter, "Erreur: Le prénom est obligatoire!");
        return;
    }
    if (!validate_name(prenom)) {
        show_popup_message(window_ajouter, "Erreur: Le prénom doit contenir uniquement des lettres!");
        return;
    }
    if (specialite == NULL || strlen(specialite) == 0) {
        show_popup_message(window_ajouter, "Erreur: Veuillez sélectionner une spécialité!");
        return;
    }
    if (!validate_date(jour, mois, annee)) {
        show_popup_message(window_ajouter, "Erreur: Date de naissance invalide!");
        return;
    }
    strcpy(e.cin_e, cin);
    strcpy(e.nom_e, nom);
    strcpy(e.prenom_e, prenom);
    strcpy(e.specialite_e, specialite);
    if (w == 1) {
        strcpy(e.sexe_e, "Homme");
    } else {
        strcpy(e.sexe_e, "Femme");
    }
    e.date_nai_e.j = jour;
    e.date_nai_e.m = mois;
    e.date_nai_e.a = annee;
    if (z == 1) {
        strcpy(e.entrainement_prive, "1");
    } else {
        strcpy(e.entrainement_prive, "0");
    }
    ajouter_entraineur(e);
    entraineur added = trouver_entraineur(cin);
    char success_msg[300];
    sprintf(success_msg, "Entraîneur ajouté avec succès!\nMot de passe généré: %s", added.mot_passe_e);
    show_success_message(window_ajouter, success_msg);
    if (global_treeview_entraineurs != NULL) {
        afficher_entraineurs(global_treeview_entraineurs);
        search_mode = 0;
    }
    gtk_widget_destroy(window_ajouter);
}



/******************* ADD FORM - ANNULER BUTTON ********************/
void on_button_ajout_annulertrainer_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *window_ajouter;
    window_ajouter = lookup_widget(GTK_WIDGET(button), "window_ajouter_entraineur");
    gtk_widget_destroy(window_ajouter);
}


/******************* MODIFY FORM - ENREGISTRER BUTTON ********************/
void on_button_mod_enregistrer_admin_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *window_modifier;
    GtkWidget *entry_cin, *entry_nom, *entry_prenom;
    GtkWidget *spinbutton_jour, *spinbutton_mois, *spinbutton_annee;
    GtkWidget *combobox_specialite;
    entraineur e;
    const char *cin, *nom, *prenom, *specialite;
    int jour, mois, annee;
    window_modifier = lookup_widget(GTK_WIDGET(button), "window_modifier_entraineur");
    entry_cin = lookup_widget(window_modifier, "entry_mod_cintrainer_admin");
    entry_nom = lookup_widget(window_modifier, "entry_mod_nomtrainer_admin");
    entry_prenom = lookup_widget(window_modifier, "entry_mod_prenomtrainer_admin");
    spinbutton_jour = lookup_widget(window_modifier, "spinbutton_mod_jourtrainer_admin");
    spinbutton_mois = lookup_widget(window_modifier, "spinbutton_mod_moistrainer_admin");
    spinbutton_annee = lookup_widget(window_modifier, "spinbutton_mod_anneetrainer_admin");
    combobox_specialite = lookup_widget(window_modifier, "combobox_mod_specialittetrainer_admin");
    cin = gtk_entry_get_text(GTK_ENTRY(entry_cin));
    nom = gtk_entry_get_text(GTK_ENTRY(entry_nom));
    prenom = gtk_entry_get_text(GTK_ENTRY(entry_prenom));
    jour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton_jour));
    mois = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton_mois));
    annee = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton_annee));
    specialite = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combobox_specialite));
    if (strlen(nom) == 0) {
        show_popup_message(window_modifier, "Erreur: Le nom est obligatoire!");
        return;
    }
    if (!validate_name(nom)) {
        show_popup_message(window_modifier, "Erreur: Le nom doit contenir uniquement des lettres!");
        return;
    }
    if (strlen(prenom) == 0) {
        show_popup_message(window_modifier, "Erreur: Le prénom est obligatoire!");
        return;
    }
    if (!validate_name(prenom)) {
        show_popup_message(window_modifier, "Erreur: Le prénom doit contenir uniquement des lettres!");
        return;
    }
    if (specialite == NULL || strlen(specialite) == 0) {
        show_popup_message(window_modifier, "Erreur: Veuillez sélectionner une spécialité!");
        return;
    }
    if (!validate_date(jour, mois, annee)) {
        show_popup_message(window_modifier, "Erreur: Date de naissance invalide!");
        return;
    }
    strcpy(e.cin_e, cin);
    strcpy(e.nom_e, nom);
    strcpy(e.prenom_e, prenom);
    strcpy(e.specialite_e, specialite);
    if (w == 1) {
        strcpy(e.sexe_e, "Homme");
    } else {
        strcpy(e.sexe_e, "Femme");
    }
    e.date_nai_e.j = jour;
    e.date_nai_e.m = mois;
    e.date_nai_e.a = annee;
    if (z == 1) {
        strcpy(e.entrainement_prive, "1");
    } else {
        strcpy(e.entrainement_prive, "0");
    }
    modifier_entraineur(e);
    show_success_message(window_modifier, "Entraîneur modifié avec succès!");
    is_modify_mode = 0;
    if (global_treeview_entraineurs != NULL) {
        afficher_entraineurs(global_treeview_entraineurs);
        search_mode = 0;
    }
    gtk_widget_destroy(window_modifier);
}

void
on_button_ajout_annulermembre_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_ajout_enregistremembrer_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mod_enregistrermembre_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mod_annulermembre_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_ajout_annulerequip_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_ajout_enregistrerequip_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mod_annulerequip_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mod_enregistrerequip_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}



void
on_button_ajout_enregistrercours_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_ajout_annulercours_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mod_enregistrercours_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mod_annulercours_admin_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_supp_cours_trainer_clicked   (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mescours_chercherid_trainer_enter
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mescours_retour_trainer_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_treeview_listcours_membre_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}


void
on_checkbutton_selectout_mescours_membre_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


void
on_button_supp_cours_membre_clicked    (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_mescours_retour_membre_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_treeview_liste_entraineur_membre_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}


void
on_checkbutton_selectionner_toutentraineur_membre_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


void
on_button_supp_entraineur_membre_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_retour_mesentraineur_membre_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_treeview_listevent_membre_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}


void
on_checkbutton_selectall_event_membre_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}




void
on_button_ajout_equipreserve_trainer_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_retour_equipdispo_trainer_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_rechercheid_equipdispo_trainer_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_confirmer_nouv_nombre_trainer_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_chercher_equipreserve_trainer_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_retour_equireservee_trainer_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_button_supp_equipreservee_trainer_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{

}





void
on_treeview_listes_du_cours_row_activated
                                        (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{

}


void
on_button_trier_date_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{

}



















/******************* inscription entraineur chiraz ********************************************************************************************/
char *selected_cours_id_str = NULL;  // Changed from int to string
int cours_search_mode = 0;

/******************* TREEVIEW SELECTION HANDLER ********************/
void on_treeview_cours_dispo_entraineur_cursor_changed(GtkTreeView *treeview, gpointer user_data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    // Free previous selection
    if (selected_cours_id_str != NULL) {
        g_free(selected_cours_id_str);
        selected_cours_id_str = NULL;
    }
    
    selection = gtk_tree_view_get_selection(treeview);
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        // Get the ID from column 0 (ID_COURS_COL)
        gtk_tree_model_get(model, &iter, 0, &selected_cours_id_str, -1);
        g_print("Selected Course ID: %s\n", selected_cours_id_str);
    }
}

/******************* MES COURS BUTTON ********************/
void on_button_mescours_entraineur_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *entry_cin;
    GtkWidget *treeview;
    GtkWidget *window;
    const char *cin;
    window = lookup_widget(GTK_WIDGET(button), "window_espace_entraineur");
    entry_cin = lookup_widget(window, "entry_cin_membre");
    treeview = lookup_widget(window, "treeview_cours_dispo_entraineur");
    if (!entry_cin || !treeview) {
        show_popup_message(window, "Erreur: Widgets introuvables!");
        return;
    }
    cin = gtk_entry_get_text(GTK_ENTRY(entry_cin));
    if (strlen(cin) == 0) {
        show_popup_message(window, "Veuillez entrer votre CIN!");
        return;
    }
    if (!validate_cin(cin)) {
        show_popup_message(window, "CIN invalide! Le CIN doit contenir 8 chiffres.");
        return;
    }
    afficher_mes_cours(cin, treeview);
    show_success_message(window, "Vos cours ont été chargés avec succès!");
}

/******************* S'INSCRIRE BUTTON ********************/
void on_button_sinscrirecours_entraineur_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *entry_cin;
    GtkWidget *treeview;
    GtkWidget *window;
    const char *cin;
    int result;
    window = lookup_widget(GTK_WIDGET(button), "window_espace_entraineur");
    entry_cin = lookup_widget(window, "entry_cin_membre");
    treeview = lookup_widget(window, "treeview_cours_dispo_entraineur");
    if (!entry_cin || !treeview) {
        show_popup_message(window, "Erreur: Widgets introuvables!");
        return;
    }
    cin = gtk_entry_get_text(GTK_ENTRY(entry_cin));
    if (strlen(cin) == 0) {
        show_popup_message(window, "Veuillez entrer votre CIN!");
        return;
    }
    if (!validate_cin(cin)) {
        show_popup_message(window, "CIN invalide! Le CIN doit contenir 8 chiffres.");
        return;
    }
    if (selected_cours_id_str == NULL || strlen(selected_cours_id_str) == 0) {
        show_popup_message(window, "Veuillez sélectionner un cours!");
        return;
    }
    
    // Pass the course ID STRING directly (not as integer!)
    result = inscrire_entraineur_cours(cin, selected_cours_id_str);
    
    switch(result) {
        case 1:
            show_success_message(window, "Inscription réussie!\nVous êtes maintenant inscrit à ce cours.");
            afficher_cours_disponibles(treeview);
            cours_search_mode = 0;
            break;
        case -1:
            show_popup_message(window, "Vous êtes déjà inscrit à ce cours!");
            break;
        case -2:
            show_popup_message(window, "Erreur: Cours introuvable!");
            break;
        case -5:
            show_popup_message(window, "Désolé, ce cours a déjà un entraîneur!");
            break;
        default:
            show_popup_message(window, "Erreur lors de l'inscription!");
            break;
    }
}

/******************* SEARCH BUTTON ********************/
void on_button_chercher_cours_entraineur_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *entry_recherche;
    GtkWidget *window;
    GtkWidget *treeview;
    const char *recherche;
    int result_count;
    
    window = lookup_widget(GTK_WIDGET(button), "window_espace_entraineur");
    entry_recherche = lookup_widget(window, "entry_recherche_cours_entraineur");
    treeview = lookup_widget(window, "treeview_cours_dispo_entraineur");
    
    if (!entry_recherche || !treeview) {
        show_popup_message(window, "Erreur: Widgets introuvables!");
        return;
    }
    
    recherche = gtk_entry_get_text(GTK_ENTRY(entry_recherche));
    
    if (strlen(recherche) == 0) {
        afficher_cours_disponibles(treeview);
        cours_search_mode = 0;
        show_success_message(window, "Liste rafraîchie! Tous les cours sont affichés.");
        return;
    }
    
    if (cours_search_mode == 1) {
        afficher_cours_disponibles(treeview);
        cours_search_mode = 0;
        gtk_entry_set_text(GTK_ENTRY(entry_recherche), "");
        show_success_message(window, "Liste rafraîchie! Tous les cours sont affichés.");
        return;
    }
    
    result_count = rechercher_cours_gtk(recherche, treeview);
    
    if (result_count > 0) {
        char message[200];
        cours_search_mode = 1;
        
        if (result_count == 1) {
            sprintf(message, "Recherche terminée!\n1 cours trouvé.\n\nCliquez à nouveau sur 'Chercher' pour rafraîchir.");
        } else {
            sprintf(message, "Recherche terminée!\n%d cours trouvés.\n\nCliquez à nouveau sur 'Chercher' pour rafraîchir.", result_count);
        }
        show_success_message(window, message);
    } else {
        show_popup_message(window, "Aucun cours trouvé!\nVeuillez essayer avec un autre critère.");
        cours_search_mode = 0;
    }
}

