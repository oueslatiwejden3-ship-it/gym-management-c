#ifndef CENTRES_H
#define CENTRES_H

#include <gtk/gtk.h>
#define MAX_CENTRES 1000

// Structure Centre
typedef struct {
    char id[7];
    char nom[300];
    char type[100];
    int capacite;
    int ouvert;          // 0 = fermé, 1 = ouvert
    int parking;         // 0 = non, 1 = oui
    int wifi;            // 0 = non, 1 = oui
    int cafeteria;       // 0 = non, 1 = oui
    char ville[200];
} Centre;

// Constantes
#define MAX_CENTRES 1000

// Fonctions de gestion des centres
int ajouter_centre(const char *filename, Centre c);
int modifier_centre(const char *filename, const char *id, Centre nouveau);
int supprimer_centre(const char *filename, const char *id);
Centre chercher_centre(const char *filename, const char *id);
int lire_centres_filtres(const char *filename, Centre *tab, int max_size, 
                         const char *type_filtre, int cap_min, int seulement_ouverts);
int trier_centres(Centre *tab, int n, int par_nom);  // 1=par nom, 0=par ville
int centre_existe(const char *filename, const char *id);

#endif // CENTRES_H
