#ifndef ENTRAINEUR_H
#define ENTRAINEUR_H
#include <gtk/gtk.h>

// Structure for Date of Birth
typedef struct {
    int j;
    int m;
    int a;
} date_naissance_e;

// Structure for Entraineur (Trainer)
typedef struct {
    int id_e;                     // ID (Auto-increment)
    char cin_e[200];              // CIN (ID)
    char nom_e[300];              // Nom (Last name)
    char prenom_e[300];           // Prenom (First name)
    char sexe_e[10];              // Sexe (Gender): "Homme" or "Femme"
    char specialite_e[300];       // Specialite (Specialty)
    date_naissance_e date_nai_e;  // Date de naissance (Birth date)
    char entrainement_prive[2];   // Entrainement privé (Private training): "1" or "0"
    char mot_passe_e[200];        // Mot de passe (Password)
} entraineur;

// CRUD Functions
void generer_mot_passe(char *mot_passe, int longueur);
void ajouter_entraineur(entraineur e);
int entraineur_existe(const char *cin);
void supprimer_entraineur(const char *cin);
void modifier_entraineur(entraineur e);
entraineur trouver_entraineur(const char *cin);

// Display Function
void afficher_entraineurs(GtkWidget *list);

// Additional Functions
void supprimer_entraineur_par_mdp(const char *mot_passe);
int get_next_id();

// Search Function
int rechercher_entraineurs(const char *critere, GtkWidget *treeview);

#endif // ENTRAINEUR_H
