#include <iostream>
#include <fstream>
#include <ctime>
#include "todolist.cpp"
#include "json.hpp"
/*
Inclusion de la librairie JSON par nlohmann, sous la forme d'un seul header. 
Elle inclut entre autres les librairies standard vector, string, iterator
qui sont utilisées dans la création des objets json et donc dans ce programme 
*/
using namespace std;
using json = nlohmann::json;

int main (int argc, char* argv []) {
    if (argc < 2) { // Teste si la fonction a reçu assez d'arguments (strcmp renvoie 0 en cas d'égalité, 1 sinon)
        cout << "No arguments given" << endl;
        return 1;
    }

    int id; //id de la tâche courante
    string str_id;
    int current_arg = 1; //indice de l'argument d'input en train d'être lu

    //Partie déterminant le fichier à ouvrir
    string config_arg = "0";
    if (strcmp(argv[1], "--config") == 0) {
        config_arg = argv[2];
        current_arg = 3;
    }
    string filename = config(config_arg);
    if (filename == "error") return 1; //Arrêt du programme si il y a eu une erreur dans config
    ifstream i;
    i.open(filename);

    if (not i.is_open()) {
        cout << "Erreur à l'ouverture du fichier " << filename << ", vérfiez s'il existe" << endl;
        return 1;
    }

    //Lecture du fichier JSON ouvert
    json* j = new json();
    try {
        i >> *j;
    }   
    catch (json::parse_error& e) { // Problème de syntaxe sur le fichier JSON
        std::cout << "Erreur de lecture du fichier JSON :" << filename  << endl;
        std::cout << "message: " << e.what() << endl;
        delete j;
        return 1;
    }

    i.close();
    

    /* 
    Ensuite, lecture du permier argument : le choix d'action
    Les actions delete et edit ont besoin d'un autre argument : l'ID de la tâche
    L'action display peut soit tout afficher, soit afficher toutes les tâches dont l'ID est spécifié ensuite (au moins un argument supplémentaire)
    L'action create n'a pas besoin d'autre argument
    */

    if (argc > current_arg + 1 && strcmp(argv[current_arg], "delete") == 0) { 
        str_id = argv[current_arg + 1];
        del(j, str_id);
        save(j);
        delete j;
        return 0; // Fin de la suppression de tâche
    }

    if (argc > current_arg + 1 && strcmp(argv[current_arg], "display") == 0) {
        current_arg ++;
        if (strcmp(argv[current_arg], "-all") == 0) {
            for (json::iterator it = (*j).begin(); it != (*j).end(); ++it) { //itération sur toutes les clés (id) du (*j)SON, pour print les tâches chacun leur tour
                if ((*j)[it.key()].is_object() && (*j)[it.key()]["ParentID"] == "0") { // 1ere condition : sert à écarter la ligne "next_id" qui est dans le JSON, 2eme : n'affiche que les tâches principales
                    print(it.key(), (*j), 0);
                }
            }
            delete j;
            return 0; // Fin de l'affichage
        }

        else {
            for (int i = current_arg; i < argc; i++) {
                str_id = argv[i];
                if ((*j)[str_id].size() == 0) {
                    cout << "Task id " << str_id << " does not exist" << endl;
                    delete j;
                    return 1;
                }
                else {
                    print(str_id, (*j), 0);
                }
            }
            delete j;
            return 0; // Fin de l'affichage
        }
    }

    if (argc > current_arg && strcmp(argv[current_arg], "create") == 0) {
        id = (*j)["next_id"]; // Le permier champ du JSON est l'id de la prochaine tâche à créer.
        str_id = to_string(id);
        create(j);
        current_arg ++;
    }

    if (argc > current_arg + 1 && strcmp(argv[current_arg], "edit") == 0) {
        current_arg ++;
        str_id = argv[current_arg];
        id = atoi(argv[current_arg]);
        if ((*j)[str_id].is_object() == 0) { // Permet d'écarter les cas où l'id n'existe pas / l'id utilisé est "next_id"
            cout << "Task id " << str_id << " does not exist" << endl;
            delete j;
            return 1;
        }
        current_arg ++;  
    }

    /*
    Après cela : le code est déjà terminé pour les actions delete et display
    Pour edit et create, on lit le reste des arguments pour modifier les attributs de la tâche 1 par 1
    */
    for (int i = current_arg; i < argc; i++) {
        if (strcmp(argv[i], "-title") == 0 && argc > i+1) {
            (*j)[str_id]["_Title_"] = argv[i+1];
            cout << "Title edited" << endl;
            i++; // On décale de 2 l'indice actuel sur lequel on itère pour la plupart des modifications suivantes
            continue;
        }

        if (strcmp(argv[i], "-description") == 0 && argc > i+1) {
            (*j)[str_id]["_description_"] = argv[i+1];
            cout << "Description edited" << endl;
            i++;
            continue;
        }

        if (strcmp(argv[i], "-priority") == 0 && argc > i+1) {
            (*j)[str_id]["priority"] = argv[i+1];
            cout << "Priority edited" << endl;
            i++;
            continue;
        }

        if (strcmp(argv[i], "-completion") == 0 && argc > i+1) {
            int percentage = atoi(argv[i+1]);
            if (percentage >= 0 && percentage <= 100) {
                (*j)[str_id]["completion"] = to_string(percentage) + " %"; //Est édité à 0 % si le pourcentage entré n'est pas un nombre
                cout << "Completion percentage edited to "<< percentage << " %" << endl;
            }
            else cout << "Invalid completion percentage : " << percentage << endl;
            i++;
            continue;
        }

        if (strcmp(argv[i], "-comment") == 0 && argc > i+1) {
            (*j)[str_id]["comments"].push_back(argv[i+1]);
            cout << "Comment added" << endl;
            i++;
            continue;
        }
        
        if (strcmp(argv[i], "-close") == 0) {
            close(j, str_id);
            cout << "Task closed" << endl;
            continue;
        }

        if (strcmp(argv[i], "-add_subtask") == 0 && argc > i+1) {
            if ((*j)[str_id]["ParentID"] != "0") {
                cout << "Task is already a subtask" << endl;
                i++;
                continue;
            }

            string str_sub_id = argv[i+1];
            if ((*j)[str_sub_id].is_object() && str_sub_id != str_id) { // La sous-tâche existe et n'est pas la tâche elle-même
                if ((*j)[str_sub_id]["ParentID"] == "0") { // Si la future sous-tâche n'a pas encore de parent
                    (*j)[str_id]["subtasks"].push_back(str_sub_id);
                    (*j)[str_sub_id]["ParentID"] = str_id;
                    cout << "Added task n°. " << str_sub_id << " as subtask of n°. " << str_id << endl;
                }

                else {
                    cout << "Subtask already has a parent task" << endl;
                }
            }

            else {
                cout << "Invalid subtask ID" << endl;
            }

            i++;
            continue;
        }
    }

    // Enfin, on réécrit sur le fichier JSON à l'aide de notre objet json que nous avons modifié, et on supprime l'objet
    save(j);
    delete j;
    return 0;
}