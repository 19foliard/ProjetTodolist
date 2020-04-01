#include <iostream>
#include <fstream>
#include <ctime>
#include "json.hpp"
using namespace std;
using json = nlohmann::json;

string config(string s) { 
    //Fonction qui renvoie le nom du fichier à ouvrir : celui du fichier config.json ou un autre si spécifié dans la ligne de commande
    fstream c;
    c.open("config.json"); // Fichier permettant de définir
    json config;
    try {
        c >> config;
    }
    catch (json::parse_error& e) { // Problème de syntaxe sur le fichier de configuration (peu probable, ne contient qu'un champ filename pour cette version)
        std::cout << "Erreur de lecture du fichier config.json :" << endl;
        std::cout << "message: " << e.what() << endl;
        return "error";
    }
    if (s != "0") {
        config["filename"] = s;
        cout << "Changed JSON save file to " << s << endl;
        return s;
    }
    return config["filename"];
}

void print(string str_id, json j, int subtask_level) {
    for (int k = 0; k < subtask_level; ++k) cout << "  "; 
    /*Je voulais intialement faire des sous-tâches de sous-tâches, mais je n'ai pas pu gérer les cycles, 
    d'où le "subtask_level" qui est resté, niveau de la tâche dans l'arbre de mes sous-tâches */
    cout << str_id << " :" << endl;

    for (json::iterator it = j[str_id].begin(); it != j[str_id].end(); ++it) {
        if (it.key() != "subtasks" && it.key() != "ParentID") {
            for (int k = 0; k < subtask_level; ++k) cout << "    "; // Les sous-tâches sont décalées vers la droite pour qu'on puisse les reconnaître
            cout << it.key() << " : " << it.value() << endl;
        }
    }
    cout << "------------------" << endl; // Fin de l'affichage de la tâche principale, début de l'affichage de ses sous-tâches
    vector<string> SubtaskList = j[str_id]["subtasks"];
    int size = SubtaskList.size();
    for (int i = 0; i<size; ++i) {
        string str_sub_id = SubtaskList[i];
        print(str_sub_id, j, subtask_level + 1); // Permet de mettre une tabulation pour distinguer la sous-tâche de sa tâche principale sans ajouter d'attribut dans le JSON
    }
}

void save(json* j) {
    ofstream o("file.json", ofstream::trunc);
    o << *j << endl;
    o.close();
}

void del(json* j, string str_id) {
    if (not (*j)[str_id].is_object()) {
        cout << "Invalid task ID : " << str_id << endl;
        return;
    }
    vector<string> Subtasks = (*j)[str_id]["subtasks"];
    int size = Subtasks.size();
    for (int i=0; i < size; ++i) { // On doit également supprimer toutes les sous-tâches de la tâche à supprimer
        string str_sub_id = Subtasks[i];
        (*j).erase(str_sub_id);
        cout << "Subtask no." << str_sub_id << " deleted" << endl;
    }
    string parent_id = (*j)[str_id]["ParentID"];
    (*j).erase(str_id); //Supprime la tâche de l'objet json pointé par j
    cout << "Task no." << str_id << " deleted" << endl;

    // Puis on supprime de la tâche parent l'ID de la tâche fille, éventuellement plusieurs fois si il y eu une erreur sur le JSON
    if ((*j)[parent_id].is_object()) { // Vérifie si la tâche a un parent et que son ID est valide
        vector<string> SubtasksParent = (*j)[parent_id]["subtasks"];
        int size = SubtasksParent.size();
        for (int i=0; i < size; ++i) {
            if (SubtasksParent[i] == str_id) {
                SubtasksParent.erase(SubtasksParent.begin() + i);
            }
        }
        (*j)[parent_id]["subtasks"] = SubtasksParent;
    }
}

void create(json* j) {
    int new_id = (*j)["next_id"]; // Le permier champ du JSON est l'id de la prochaine tâche à créer.
    string new_str_id = to_string(new_id);

    int next_id = new_id + 1;
    (*j)["next_id"] = next_id;
    
    time_t now = time(0);
    string date = ctime(&now);
    date.pop_back(); // Enlève le retour à la ligne créé par ctime

    (*j)[new_str_id] = {           // Création de la nouvelle tâche
        {"_Title_", "Title not defined"},
        {"_description_", "Description not defined"},
        {"priority", "Medium"},
        {"ParentID", "0"},
        {"subtasks", json::array()}, // On doit spécifier le type de l'objet vide que l'on rajoute, ici une liste
        {"comments", json::array()},
        {"date_begin", date},
        {"date_end", "TBD"}, // Date de fin "To Be Defined" (définie lorsqu'on décide de fermer la tâche)
        {"status", "Open"},
        {"completion", "0 %"}
    };
    cout << "Task added, id : " << new_id << endl;
}

void close(json* j, string str_id) {
    if ((*j)[str_id]["status"] == "Closed") {
        cout << "Task already closed" << endl;
        return;
    }
    time_t now = time(0);
    string date = ctime(&now);
    date.pop_back();

    (*j)[str_id]["status"] = "Closed";
    (*j)[str_id]["date_end"] = date;

    if ((*j)[str_id]["ParentID"] == "0") { // Si la tâche n'est pas sous-tâche de quelque chose d'autre
        vector<string> Subtasks = (*j)[str_id]["subtasks"];
        int size = Subtasks.size();
        for (int i=0; i < size; ++i) { // On doit également fermer toutes les sous-tâches de la tâche à fermer
            string str_sub_id = Subtasks[i];
            close(j, str_sub_id);
        }
    }
}