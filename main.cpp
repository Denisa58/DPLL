#include <iostream>
#include <fstream>
#include <cstdlib>
#include <windows.h>
#include <chrono>
#include <iomanip>

#define MAX_CLAUZE 10000
#define MAX_LITERALI 100
#define MAX_LUNGIME_CLAUZA 50

using namespace std;
using namespace std::chrono;

int clauze[MAX_CLAUZE][MAX_LUNGIME_CLAUZA];
int lungime[MAX_CLAUZE];
int nrClauze = 0;
int nrVariabile = 0;

// Functie pentru masurarea memoriei folosite de proces (in GB)
double memorieFolositaGB() {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    DWORDLONG memTotal = statex.ullTotalPhys;
    DWORDLONG memUsed = memTotal - statex.ullAvailPhys;
    return memUsed / (1024.0 * 1024.0 * 1024.0);
}

// Functie pentru citirea clauzelor din fisier .cnf
void citireClauze(const char* numeFisier) {
    ifstream fin(numeFisier);
    if (!fin) {
        cout << "Eroare la deschiderea fisierului!\n";
        exit(1);
    }

    char c;
    string linie;
    while (fin >> c) {
        if (c == 'c') {
            getline(fin, linie);
            continue;
        }
        if (c == 'p') {
            fin >> linie;
            int vars, clauses;
            fin >> vars >> clauses;
            nrVariabile = vars;
            continue;
        }

        fin.putback(c);
        int literal, index = 0;
        while (fin >> literal) {
            if (literal == 0) break;
            if (index < MAX_LUNGIME_CLAUZA) {
                clauze[nrClauze][index++] = literal;
            }
        }
        lungime[nrClauze] = index;
        nrClauze++;
        if (nrClauze >= MAX_CLAUZE) {
            cout << "Eroare: S-a depasit limita maxima de clauze!" << endl;
            exit(1);
        }
    }

    cout << "Clauze citite: " << nrClauze << endl;
    cout << "Numar variabile: " << nrVariabile << endl;
}

// Functie pentru a verifica daca formula este satisfacuta
bool esteSatisfacuta(bool valori[]) {
    for (int i = 0; i < nrClauze; i++) {
        bool satisfacuta = false;
        for (int j = 0; j < lungime[i]; j++) {
            int lit = clauze[i][j];
            int var = abs(lit);
            if (var == 0 || var > nrVariabile) continue;
            bool val = (lit > 0) ? valori[var - 1] : !valori[var - 1];
            if (val) {
                satisfacuta = true;
                break;
            }
        }
        if (!satisfacuta) return false;
    }
    return true;
}

// Functie care face asignare si modifica formula
void aplicaAsignare(int literal, bool valori[], bool clauzaEliminata[], bool literalEliminat[][MAX_LUNGIME_CLAUZA]) {
    int var = abs(literal);
    valori[var - 1] = (literal > 0);

    for (int i = 0; i < nrClauze; i++) {
        if (clauzaEliminata[i]) continue;

        bool clauzaSatisfacuta = false;
        for (int j = 0; j < lungime[i]; j++) {
            if (literalEliminat[i][j]) continue;
            if (clauze[i][j] == literal) {
                clauzaSatisfacuta = true;
                break;
            }
        }

        if (clauzaSatisfacuta) {
            clauzaEliminata[i] = true;
        } else {
            for (int j = 0; j < lungime[i]; j++) {
                if (clauze[i][j] == -literal) {
                    literalEliminat[i][j] = true;
                }
            }
        }
    }
}

// Functie care face backtrack
void undoAsignare(int literal, bool valori[], bool clauzaEliminata[], bool literalEliminat[][MAX_LUNGIME_CLAUZA]) {
    int var = abs(literal);
    valori[var - 1] = false; // revenim la necunoscut

    for (int i = 0; i < nrClauze; i++) {
        if (clauzaEliminata[i]) {
            clauzaEliminata[i] = false;
        } else {
            for (int j = 0; j < lungime[i]; j++) {
                if (literalEliminat[i][j] &&
                    (clauze[i][j] == -literal)) {
                    literalEliminat[i][j] = false;
                }
            }
        }
    }
}

// Functie principala DPLL (cu backtracking manual)
bool dpll(bool valori[], bool clauzaEliminata[], bool literalEliminat[][MAX_LUNGIME_CLAUZA]) {
    // Verifica daca toate clauzele sunt satisfacute
    bool toateEliminate = true;
    for (int i = 0; i < nrClauze; i++) {
        if (!clauzaEliminata[i]) {
            toateEliminate = false;
            break;
        }
    }
    if (toateEliminate) return true;

    // Verifica daca exista clauza goala (esec)
    for (int i = 0; i < nrClauze; i++) {
        if (clauzaEliminata[i]) continue;
        bool goala = true;
        for (int j = 0; j < lungime[i]; j++) {
            if (!literalEliminat[i][j]) {
                goala = false;
                break;
            }
        }
        if (goala) return false;
    }

    // Alege o variabila nerezolvata
    for (int var = 1; var <= nrVariabile; var++) {
        if (valori[var - 1] != true && valori[var - 1] != false) {
            // Tenta True
            aplicaAsignare(var, valori, clauzaEliminata, literalEliminat);
            if (dpll(valori, clauzaEliminata, literalEliminat)) {
                return true;
            }
            undoAsignare(var, valori, clauzaEliminata, literalEliminat);

            // Tenta False
            aplicaAsignare(-var, valori, clauzaEliminata, literalEliminat);
            if (dpll(valori, clauzaEliminata, literalEliminat)) {
                return true;
            }
            undoAsignare(-var, valori, clauzaEliminata, literalEliminat);

            return false;
        }
    }

    return false;  // fallback
}

int main() {
    const char* numeFisier = "formula.cnf";
    citireClauze(numeFisier);

    bool valori[MAX_LITERALI] = {0}; // false / true / necunoscut
    bool clauzaEliminata[MAX_CLAUZE] = {0};
    bool literalEliminat[MAX_CLAUZE][MAX_LUNGIME_CLAUZA] = {0};

    auto start = steady_clock::now();
    bool rezultat = dpll(valori, clauzaEliminata, literalEliminat);
    auto end = steady_clock::now();

    auto total_time = duration_cast<seconds>(end - start).count();
    double memMB = memorieFolositaGB();

    cout << endl << "Rezultatul: " << (rezultat ? "Satisfiabil" : "Nesatisfiabil") << endl;
    cout << "Timp total de executie: " << total_time << " sec" << endl;
    cout << "Memorie folosita la final: " << fixed << setprecision(2) << memMB << " MB" << endl;

    return 0;
}
