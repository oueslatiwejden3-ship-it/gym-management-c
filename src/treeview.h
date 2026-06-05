#ifndef TREEVIEW_H
#define TREEVIEW_H

#include <gtk/gtk.h>

// Structures
#define MAX_STR 100
#define MAX_COURS 1000
#define MAX_CENTRES 1000



typedef struct {
    char id_cours[MAX_STR];
    char nom_cours[MAX_STR];
    char type_activite[MAX_STR];
    int jour;
    int mois;
    int annee;
    int heure_debut;
    int heure_fin;
    int niveau; // 0=Debutant, 1=Intermediaire, 2=Avance
    int places_max;
    char centre[MAX_STR];
    unsigned int jours_mask;
} Cours;

typedef struct {
    int CIN_trainer;
    char Nom_trainer[100];
    char Prenom_trainer[100];
    int sexe_trainer; //0=homme 1=femme
    char specialite[100];
    int jour;
    int mois;
    int annee;
    int train_priv;
} Entraineur;

typedef struct {
    char id_equipement[MAX_STR];
    char nom_equipement[MAX_STR];
    char type[MAX_STR];
    int nombre;
} Equipement;
/**************************************************************/

typedef struct {
    int j;
    int m;
    int a;
} date_event;

// Structure for Evenement (Event)
typedef struct {
    int id_event;                    // ID (Auto-increment)
    char nom_event[300];             // Nom d'événement (Event name)
    char type_event[100];            // Type d'événement (Event type)
    char centre[200];                // Centre (Gym/Center)
    date_event date_e;               // Date d'événement (Event date)
    char statut[20];                 // Statut: "en cours", "terminer", "Annulé"
} evenement;

// Structure for Event Enrollment (Inscription)
typedef struct {
    int id_inscription;              // ID (Auto-increment)
    char cin_membre[20];             // CIN of the member
    int id_event;                    // ID of the event
    char nom_event[300];             // Event name (for display)
    char type_event[100];            // Event type (for display)
    char centre[200];                // Centre (for display)
    date_event date_e;               // Event date (for display)
} inscription_event;

// CRUD Functions for Events
void ajouter_evenement(evenement ev);
int evenement_existe(int id_event);
void supprimer_evenement(int id_event);
void modifier_evenement(evenement ev);
evenement trouver_evenement(int id_event);
void afficher_evenements(GtkWidget *treeview);
int rechercher_evenements(const char *critere, GtkWidget *treeview);
int get_next_event_id();

// Functions for Event Enrollments
void ajouter_inscription_event(inscription_event insc);
void supprimer_inscription_event(int id_inscription);
int inscription_existe(const char *cin_membre, int id_event);
void afficher_inscriptions_membre(GtkWidget *treeview, const char *cin_membre);
int rechercher_mes_inscriptions(const char *critere, GtkWidget *treeview, const char *cin_membre);
int get_next_inscription_id();

// Helper Function
void afficher_entraineurs(GtkWidget *treeview);
/********************************************************************/
typedef struct {
    char cin[20];
    char nom[50];
    char prenom[50];
    char sexe[10];
    char telephone[20];
    char date_inscription[20];
} Membre;

typedef struct {
    int cin;
    char nom[100];
    char prenom[100];
    char specialite[100];
    char jours_disponibles[20]; // L M Me J V S format
} EntraineurPrive;

// Function declarations to add in treeview.h
void refresh_admin_membres_treeview(GtkWidget *treeview);
void refresh_membre_entraineurs_prives_treeview(GtkWidget *treeview);

// Event functions declarations
void refresh_admin_events_treeview(GtkWidget *treeview);
void refresh_member_events_treeview(GtkWidget *treeview);
// Treeview functions declarations
void refresh_trainer_courses_treeview(GtkWidget *liste, const char *id_entraineur_param);
void refresh_equipment_treeview(GtkWidget *liste);
void refresh_equipment_types_combobox(GtkWidget *combobox, int ajouter_aucune);
void refresh_activity_types_combobox(GtkWidget *combobox);
void refresh_admin_courses_treeview(GtkWidget *treeview);
void refresh_member_courses_treeview(GtkWidget *treeview);

// New centres functions
void refresh_admin_centres_treeview(GtkWidget *treeview);
void refresh_trainer_centres_treeview(GtkWidget *treeview);
void refresh_centre_types_combobox(GtkWidget *combobox, const char *filename);
void refresh_trainer_centre_types_combobox(GtkWidget *combobox, const char *filename);
//TRAINERS TREEVIEW
void refresh_admin_trainers_treeview(GtkWidget *treeview);

void refresh_admin_membres_treeview(GtkWidget *treeview);
void refresh_membre_entraineurs_prives_treeview(GtkWidget *treeview);

/**************************chirazz centre *****************************/

typedef struct {
    int id_inscription_trainer;
    char cin_trainer[20];
    char id_cours[MAX_STR];
} inscription_trainer_cours;


int entraineur_a_deja_un_cours(const char *cin_trainer) ;
int inscrire_entraineur_cours(const char *cin_trainer, const char *id_cours);
int trainer_deja_inscrit(const char *cin_trainer, const char *id_cours);
int compter_places_prises(const char *id_cours);
int get_next_inscription_trainer_id();
int cours_deja_avec_entraineur(const char *id_cours);

/* GTK Display functions */
void afficher_cours_disponibles(GtkWidget *treeview);
void afficher_mes_cours(const char *cin_trainer, GtkWidget *treeview);
int rechercher_cours_gtk(const char *critere, GtkWidget *treeview);

#endif /* TREEVIEW_H */
