#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "centres.h"

// Vérifier si un centre existe
int centre_existe(const char *filename, const char *id) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) return 0;
    
    char line[512];
    Centre c;
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        
        if (sscanf(line, "%[^,],%*[^,],%*[^,],%*d,%*d,%*d,%*d,%*d,%*[^\n]", c.id) == 1) {
            if (strcmp(c.id, id) == 0) {
                fclose(f);
                return 1;
            }
        }
    }
    
    fclose(f);
    return 0;
}

// Ajouter un centre
int ajouter_centre(const char *filename, Centre c) {
    // Vérifier si l'ID existe déjà
    if (centre_existe(filename, c.id)) {
        return 0; // ID existe déjà
    }
    
    FILE *f = fopen(filename, "a");
    if (f == NULL) return 0;
    
    fprintf(f, "%s,%s,%s,%d,%d,%d,%d,%d,%s\n",
            c.id, c.nom, c.type, c.capacite, c.ouvert,
            c.parking, c.wifi, c.cafeteria, c.ville);
    
    fclose(f);
    return 1;
}

// Chercher un centre par ID
Centre chercher_centre(const char *filename, const char *id) {
    Centre c;
    strcpy(c.id, "-1"); // Valeur par défaut pour "non trouvé"
    
    FILE *f = fopen(filename, "r");
    if (f == NULL) return c;
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        
        int result = sscanf(line, "%[^,],%[^,],%[^,],%d,%d,%d,%d,%d,%[^\n]",
                  c.id, c.nom, c.type, &c.capacite, &c.ouvert,
                  &c.parking, &c.wifi, &c.cafeteria, c.ville);
        
        if (result == 9 && strcmp(c.id, id) == 0) {
            fclose(f);
            return c;
        }
    }
    
    fclose(f);
    strcpy(c.id, "-1");
    return c;
}

// Modifier un centre
int modifier_centre(const char *filename, const char *id, Centre nouveau) {
    FILE *f = fopen(filename, "r");
    FILE *temp = fopen("temp_centres.txt", "w");
    
    if (f == NULL || temp == NULL) {
        if (f) fclose(f);
        if (temp) fclose(temp);
        return 0;
    }
    
    Centre c;
    char line[512];
    int trouve = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') {
            fprintf(temp, "%s", line);
            continue;
        }
        
        int result = sscanf(line, "%[^,],%[^,],%[^,],%d,%d,%d,%d,%d,%[^\n]",
                  c.id, c.nom, c.type, &c.capacite, &c.ouvert,
                  &c.parking, &c.wifi, &c.cafeteria, c.ville);
        
        if (result == 9 && strcmp(c.id, id) == 0) {
            // Écrire les nouvelles données
            fprintf(temp, "%s,%s,%s,%d,%d,%d,%d,%d,%s\n",
                    nouveau.id, nouveau.nom, nouveau.type, nouveau.capacite,
                    nouveau.ouvert, nouveau.parking, nouveau.wifi,
                    nouveau.cafeteria, nouveau.ville);
            trouve = 1;
        } else {
            fprintf(temp, "%s", line);
        }
    }
    
    fclose(f);
    fclose(temp);
    
    if (trouve) {
        remove(filename);
        rename("temp_centres.txt", filename);
        return 1;
    } else {
        remove("temp_centres.txt");
        return 0;
    }
}

// Supprimer un centre
int supprimer_centre(const char *filename, const char *id) {
    FILE *f = fopen(filename, "r");
    FILE *temp = fopen("temp_centres.txt", "w");
    
    if (f == NULL || temp == NULL) {
        if (f) fclose(f);
        if (temp) fclose(temp);
        return 0;
    }
    
    Centre c;
    char line[512];
    int trouve = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') {
            fprintf(temp, "%s", line);
            continue;
        }
        
        int result = sscanf(line, "%[^,],%*[^,],%*[^,],%*d,%*d,%*d,%*d,%*d,%*[^\n]", c.id);
        
        if (result == 1 && strcmp(c.id, id) == 0) {
            trouve = 1;
            // Ne pas écrire cette ligne (suppression)
        } else {
            fprintf(temp, "%s", line);
        }
    }
    
    fclose(f);
    fclose(temp);
    
    if (trouve) {
        remove(filename);
        rename("temp_centres.txt", filename);
        return 1;
    } else {
        remove("temp_centres.txt");
        return 0;
    }
}

// Lire les centres avec filtres
int lire_centres_filtres(const char *filename, Centre *tab, int max_size,
                         const char *type_filtre, int cap_min, int seulement_ouverts) {
    FILE *f = fopen(filename, "r");
    int count = 0;
    
    if (f == NULL) return 0;
    
    char line[512];
    Centre c;
    
    while (count < max_size && fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        
        int result = sscanf(line, "%[^,],%[^,],%[^,],%d,%d,%d,%d,%d,%[^\n]",
                  c.id, c.nom, c.type, &c.capacite, &c.ouvert,
                  &c.parking, &c.wifi, &c.cafeteria, c.ville);
        
        if (result == 9) {
            // Appliquer les filtres
            int accepte = 1;
            
            // Filtre par type
            if (type_filtre != NULL && strlen(type_filtre) > 0 && 
                strcmp(type_filtre, "Tous les types") != 0) {
                if (strcmp(c.type, type_filtre) != 0) {
                    accepte = 0;
                }
            }
            
            // Filtre par capacité minimale
            if (c.capacite < cap_min) {
                accepte = 0;
            }
            
            // Filtre centres ouverts uniquement
            if (seulement_ouverts && c.ouvert != 1) {
                accepte = 0;
            }
            
            if (accepte) {
                tab[count] = c;
                count++;
            }
        }
    }
    
    fclose(f);
    return count;
}

// Fonction de comparaison pour le tri par nom
static int comparer_nom(const void *a, const void *b) {
    Centre *c1 = (Centre *)a;
    Centre *c2 = (Centre *)b;
    return strcmp(c1->nom, c2->nom);
}

// Fonction de comparaison pour le tri par ville
static int comparer_ville(const void *a, const void *b) {
    Centre *c1 = (Centre *)a;
    Centre *c2 = (Centre *)b;
    return strcmp(c1->ville, c2->ville);
}

// Trier les centres
int trier_centres(Centre *tab, int n, int par_nom) {
    if (tab == NULL || n <= 0) return 0;
    
    if (par_nom) {
        qsort(tab, n, sizeof(Centre), comparer_nom);
    } else {
        qsort(tab, n, sizeof(Centre), comparer_ville);
    }
    
    return 1;
}
///////////////////////

int charger_centres(const char *filename, Centre centres[], int *nb_centres) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("DEBUG: Could not open centres file: %s\n", filename);
        /* Create empty file if it doesn't exist */
        file = fopen(filename, "w");
        if (file) fclose(file);
        *nb_centres = 0;
        return 0;
    }
    
    *nb_centres = 0;
    char line[256];
    
    while (fgets(line, sizeof(line), file) && *nb_centres < MAX_CENTRES) {
        if (line[0] == '#' || line[0] == '\n') continue;
        
        Centre *c = &centres[*nb_centres];
        int ouvert_int, parking_int, wifi_int, cafeteria_int;
        
        int result = sscanf(line, "%99[^,],%99[^,],%99[^,],%d,%d,%d,%d,%d,%99[^\n]",
                           c->id, c->nom, c->type, &c->capacite,
                           &ouvert_int, &parking_int, &wifi_int, &cafeteria_int, c->ville);
        
        if (result == 9) {
            c->ouvert = ouvert_int;
            c->parking = parking_int;
            c->wifi = wifi_int;
            c->cafeteria = cafeteria_int;
            (*nb_centres)++;
        }
    }
    
    fclose(file);
    printf("DEBUG: Loaded %d centres from %s\n", *nb_centres, filename);
    return 0;
}

int sauvegarder_centres(const char *filename, Centre centres[], int nb_centres) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening centres file for writing");
        return -1;
    }
    
    fprintf(file, "# ID,Nom,Type,Capacité,Ouvert,Parking,Wifi,Cafeteria,Ville\n");
    
    for (int i = 0; i < nb_centres; i++) {
        fprintf(file, "%s,%s,%s,%d,%d,%d,%d,%d,%s\n",
                centres[i].id,
                centres[i].nom,
                centres[i].type,
                centres[i].capacite,
                centres[i].ouvert,
                centres[i].parking,
                centres[i].wifi,
                centres[i].cafeteria,
                centres[i].ville);
    }
    
    fclose(file);
    printf("DEBUG: Saved %d centres to %s\n", nb_centres, filename);
    return 0;
}

