#include <iostream>
#include <string>
#include <cctype>
#include <sstream>
#include <functional>
#include <stdexcept>
#include <vector>
#include <list>
#include <unordered_map>
#include <algorithm>

using namespace std;

class Medic;
class Pacient;
class Programare;
class Reteta;
class IdObiect;

//clasa exceptii general
class ClinicaException : public runtime_error {
public:
    ClinicaException(const string& msg) : runtime_error(msg) {}
};

// exceptie pentru validare (ex cnp, ora, data)
class ValidareException : public ClinicaException {
    string camp;
    string valoare;
public:
    ValidareException(const string& camp, const string& valoare, const string& motiv) : ClinicaException("Validare esuata pentru " + camp + " (valoare='" + valoare + "'): " + motiv), camp(camp), valoare(valoare) {}

    const string& getCamp() const noexcept {
        return camp;
    }
    const string& getValoare() const noexcept {
        return valoare;
    }

    const char* what() const noexcept override {
        return runtime_error::what();
    }
};

//exceptie pentru entitate inexistenta
class EntitateInexistentaException : public ClinicaException {
    string tipEntitate;
    string identificator;
public:
    EntitateInexistentaException(const string& tip, const string& id) : ClinicaException(tip + " inexistent: " + id), tipEntitate(tip), identificator(id) {}

    const string& getTipEntitate() const noexcept {
        return tipEntitate;
    }
    const string& getIdentificator() const noexcept {
        return identificator;
    }

    const char* what() const noexcept override {
        return runtime_error::what();
    }
};

// exceptie pentru refuzare acces medic
class AccesRefuzatException : public ClinicaException {
    int idMedic;
public:
    AccesRefuzatException(int idMedic, const string& detaliu) : ClinicaException("Acces refuzat pentru medicul cu id=" + to_string(idMedic) + ": " + detaliu), idMedic(idMedic) {}

    int getIdMedic() const noexcept {
        return idMedic;
    }

    const char* what() const noexcept override {
        return runtime_error::what();
    }
};

// singleton
class Jurnal {
private:
    vector<string> jurnal;
    Jurnal() {}

public:
    Jurnal(const Jurnal&) = delete;
    Jurnal& operator=(const Jurnal&) = delete;

    static Jurnal& getInstance() {
        static Jurnal instance;
        return instance;
    }

    void log(const string& mesaj) {
        jurnal.push_back(mesaj);
    }

    void afiseaza() const {
        cout << "----- AGENDA CLINICA ------" << endl;
        if (jurnal.empty()) {
            cout << "Jurnal gol!" << endl;
        }
        else {
            for (int i = 0; i < (int)jurnal.size(); i++) {
                cout << (i + 1) << ". " << jurnal[i] << endl;
            }
        }
        cout << "---------------------------" << endl;
    }
};

// clasa abstracta pentru id unic
class IdObiect {
    static int idGenerator;
protected:
    int id;
public:
    IdObiect() : id(++idGenerator) {}
    virtual ~IdObiect() = default;

    int getId() const noexcept {
        return id;
    }
    virtual string toString() const = 0;

    friend ostream& operator<<(ostream& out, const IdObiect& obj) {
        return out << obj.toString();
    }

    bool operator==(const IdObiect& other) const {
        return id == other.id;
    }
};
int IdObiect::idGenerator = 0;


// clasa template pentru vectori
template<typename T>
class Depozit {
private:
    vector<T*> elemente;

public:
    Depozit() = default;

    ~Depozit() {
        for (T* e : elemente)
            delete e;
    }

    Depozit(const Depozit&) = delete;
    Depozit& operator=(const Depozit&) = delete;

    void adauga(T* element) {
        if (element == nullptr)
            throw ClinicaException("Nu se poate adauga element nullptr");
        elemente.push_back(element);
    }

    int size() const noexcept {
        return (int)elemente.size();
    }

    T* index(int index) const {
        if (index < 0 || index >= (int)elemente.size())
            throw ClinicaException("Index invalid: " + to_string(index));
        return elemente[index];
    }

    void stergeDupaPointer(T* p) {
        auto it = find(elemente.begin(), elemente.end(), p);
        if (it == elemente.end())
            throw EntitateInexistentaException("Element", "(pointer)");
        delete *it;
        elemente.erase(it);
    }

    template<typename Element>
    T* gaseste(Element el) const {
        auto it = find_if(elemente.begin(), elemente.end(), el);
        return (it != elemente.end()) ? *it : nullptr;
    }

    template<typename Element>
    int numara(Element el) const {
        return (int)count_if(elemente.begin(), elemente.end(), el);
    }

    const vector<T*>& getAll() const noexcept {
        return elemente;
    }

    void clearOnly() noexcept {
        elemente.clear();
    }
};

template<typename Container, typename Compare>
void sorteaza(Container& col, Compare comp) {
    sort(col.begin(), col.end(), comp);
}

template<typename Container, typename Predicate>
int numaraConform(const Container& col, Predicate pred) {
    return (int)count_if(col.begin(), col.end(), pred);
}

// Am specializat functia pentru vector<int> pentru a optimiza cautarea
// fara a folosi iteratori standard
template<>
int numaraConform(const vector<int>& col, function<bool(int)> pred) {
    int cnt = 0;
    for (int v : col)
        if (pred(v))
            cnt++;
    return cnt;
}

template<typename T, int N>
class ColectieFixa {
private:
    T elemente[N];
    int dimensiune = 0;
public:
    void adauga(const T& el) {
        if (dimensiune >= N)
            throw overflow_error("ColectieFixa plin (limita " + to_string(N) + ")");
        elemente[dimensiune++] = el;
    }
    int size() const noexcept {
        return dimensiune;
    }
    static constexpr int capacitate() noexcept {
        return N;
    }

    const T& at(int i) const {
        if (i < 0 || i >= dimensiune)
            throw out_of_range("Index invalid in ColectieFixa");
        return elemente[i];
    }
};

// clasa abstracta (baza pentru medic si pacient)
class Persoana: public IdObiect{
protected:
    string nume;
    int varsta;
    static int nrPersoaneTotal;

private:
    long long idIntern;

public:
    Persoana() : nume(""), varsta(0), idIntern(++nrPersoaneTotal) {}

    Persoana(const string& nume, int varsta): nume(nume), varsta(varsta), idIntern(++nrPersoaneTotal) {}

    Persoana(const Persoana& p): nume(p.nume), varsta(p.varsta), idIntern(++nrPersoaneTotal) {}

    Persoana& operator=(const Persoana& p) {
        if (this == &p)
            return *this;
        nume = p.nume;
        varsta = p.varsta;
        return *this;
    }

    virtual ~Persoana() = default;

    virtual void afiseaza() const = 0;
    virtual const char* getRol() const = 0;
    virtual string toString() const override = 0;

    void afiseazaInfoComuna() const {
        cout << "Nume: " << nume << " | Varsta: " << varsta;
    }

    //getteri
    const string& getNume() const {
        return nume;
    }
    int getVarsta() const {
        return varsta;
    }
    long long getIdIntern() const {
        return idIntern;
    }
    static int getNrPersoaneTotal() {
        return nrPersoaneTotal;
    }

    //setteri
    void setNume(const string& nume) {
        this->nume = nume;
    }
    void setVarsta(int v) {
        varsta = v;
    }
};
int Persoana::nrPersoaneTotal = 0;


class Medic : public Persoana {
private:
    const int idMedic;

    char specializare;        // De exemplu : "C" - Cardiologie, "N" - Neurologie, "P" - Pediatrie, "O" - Oftalmologie
    float salariu;
    int nrProgramari;        // o sa poata fi modificat doar atunco cand se creaza in clasa programare o programare la medicul respectiv
    bool activ;

    static int nrMedici;
    static vector<Medic*> medici; // Ales pentru acces iterativ rapid O(N) si adaugare O(1)

public:
    // constructor fara parametri
    Medic() : Persoana(), idMedic(++nrMedici), specializare('-'), salariu(0.0f), nrProgramari(0), activ(true) {
        cout << "S-a creat medicul cu id=" << idMedic << endl;
    }

    // constructor cu toti parametrii
    Medic(const string& nume, int varsta, char specializare, float salariu, bool activ): Persoana(nume, varsta), idMedic(++nrMedici), specializare(specializare), salariu(salariu), nrProgramari(0), activ(activ) {
        cout << "S-a creat medicul cu numele " << nume << " si id=" << idMedic << endl;
        adaugaMedic(this);
        Jurnal::getInstance().log("Adaugat medic: " + nume);
    }

    // constructor cu 3 parametri
    Medic(const string& nume, char specializare, float salariu): Persoana(nume, 0), idMedic(++nrMedici), specializare(specializare), salariu(salariu), nrProgramari(0), activ(true) {
        cout << "S-a creat medicul cu numele " << nume << " si id=" << idMedic << endl;
        adaugaMedic(this);
    }

    // constructor cu 2 parametri
    Medic(const string& nume, char specializare): Persoana(nume, 0), idMedic(++nrMedici), specializare(specializare), salariu(0.0f), nrProgramari(0), activ(true) {
        cout << "S-a creat medicul cu numele " << nume << " si id=" << idMedic << endl;
        adaugaMedic(this);
    }

    // constructor cu 1 parametru (explicit)
    explicit Medic(const string& nume): Persoana(nume, 0), idMedic(++nrMedici), specializare('-'), salariu(0.0f), nrProgramari(0), activ(true) {
        cout << "S-a creat medicul cu numele " << nume << " si id=" << idMedic << endl;
        adaugaMedic(this);
    }

    // copy constructor
    Medic(const Medic& m): Persoana(m), idMedic(++nrMedici), specializare(m.specializare), salariu(m.salariu), nrProgramari(m.nrProgramari), activ(m.activ) {
    }

    //  destructor (override)
    ~Medic() override {
        cout << "S-a eliminat medicul cu id=" << idMedic << endl;
    }

    //  suprascrierea operatorului =
    Medic& operator=(const Medic& m) {
        if (this == &m)
            return *this;
        Persoana::operator=(m);
        specializare = m.specializare;
        salariu = m.salariu;
        nrProgramari = m.nrProgramari;
        activ = m.activ;
        return *this;
    }

    // operator [] - returneaza caracterul la pozitia indicata din nume
    char operator[](int index) const {
        if (index >= 0 && index < (int)nume.length())
            return nume[index];
        cout << "Indexul " << index << "este invalid prntru numele curent!" << endl;
        return '\0';
    }

    //opereatorii de citire/afisare
    friend ostream& operator<<(ostream& out, const Medic& m);
    friend istream& operator>>(istream& in, Medic& m);


    string toString() const override {
        ostringstream oss;
        oss << "Medic[id=" << getId() << ", nume=" << nume
            << ", spec=" << specializare << ", salariu=" << salariu << "]";
        return oss.str();
    }

    //doi medici sunt egali daca au acelasi id
    bool operator==(const Medic& medic){
        return this->idMedic == medic.idMedic;
    }

    //comparam medicii din punct de vedere al salariului
    bool operator<=(const Medic& medic){
        return this->salariu <= medic.salariu;
    }

    // operator ++ (prefix) - bonus 100 la salariu
    Medic& operator++() {
        salariu += 100;
        return *this;
    }
    // operator ++ (postfix)
    Medic operator++(int) {
        Medic temp(*this);
        salariu += 100;
        return temp;
    }

    // oprator -- - scadem 100 lei din salariu
    Medic& operator--(){
        this->salariu -= 100;
        return *this;
    }

    //daca adunam un medic cu un int, adaugam cantitatea la salariu
    float operator+(int c){
        return this->salariu + c;
    }

    //daca scadem un medic cu un int, scadem din salariu cantitatea
    float operator-(int c){
        return this->salariu - c;
    }

    // suprascriere functii virtuale + override
    void afiseaza() const override {
        cout << "MEDIC ID=" << idMedic << " | ";
        afiseazaInfoComuna();
        cout << " | Spec=" << specializare
             << " | Salariu=" << salariu
             << " | Programari=" << nrProgramari
             << " | Activ=" << (activ ? "Da" : "Nu") << endl;
    }
    const char* getRol() const override {
        return "Medic";
    }

    // getteri (cerinta P1-3)
    int getIdMedic() const {
        return idMedic;
    }
    char getSpecializare() const {
        return specializare;
    }
    float getSalariu() const {
        return salariu;
    }
    int getNrProgramari() const {
        return nrProgramari;
    }
    bool getActiv() const {
        return activ;
    }
    static int getSizeMedici() { return (int)medici.size(); }
    static int getNrMedici() {
        return nrMedici;
    }
    static const vector<Medic*>& getMedici() { return medici; }

    // setteri
    void setSpecializare(char s) {
        specializare = s;
    }
    void setSalariu(float s) {
        salariu = s;
    }
    void setActiv(bool a) {
        activ = a;
    }


    void incrementeazaProgramari() {
        nrProgramari++;
    }
    void decrementeazaProgramari() {
        if (nrProgramari > 0)
            nrProgramari--;
    }

   // adauga medici in vector
    static void adaugaMedic(Medic* m) {
        if (m == nullptr) throw ClinicaException("Medic nullptr nu se poate adauga");
        medici.push_back(m);
    }

    // strge vectorul
    static void stergeMedici() {
        for (Medic* m : medici) delete m;
        medici.clear();
        nrMedici = 0;
    }

    static Medic* gasesteMedicDupaId(int id) {
        auto it = find_if(medici.begin(), medici.end(),
                          [id](Medic* m){ return m->getIdMedic() == id; });
        if (it == medici.end())
            throw EntitateInexistentaException("Medic", "id=" + to_string(id));
        return *it;
    }

    static Medic* gasesteMedicDupaNume(const string& nume) {
        auto it = find_if(medici.begin(), medici.end(),
                          [&nume](Medic* m){ return m->getNume() == nume; });
        return (it != medici.end()) ? *it : nullptr;
    }

    // functii pentru meniu
    static void afiseazaTotiMedicii() {
        if (medici.empty()) {
            cout << "Nu exista medici!" << endl;
            return;
        }
        for_each(medici.begin(), medici.end(), [](Medic* m){ cout << *m << endl; });
    }

    static void cautaMedic() {
        char nume[256];
        cout << "Numele medicului: ";
        cin >> ws;
        cin.getline(nume, 256);
        Medic* m = gasesteMedicDupaNume(nume);
        if (m)
            cout << *m << endl;
        else
            cout << "Medic inexistent: " << nume << endl;
    }

    static void afiseazaSalariu() {
        char nume[256];
        cout << "Numele medicului: ";
        cin >> ws;
        cin.getline(nume, 256);
        Medic* m = gasesteMedicDupaNume(nume);
        if (m)
            cout << "Salariu = " << m->salariu << " RON" << endl;
        else
            cout << "Medic inexistent: " << nume << endl;
    }

    static void modificaSalariu() {
        if ((int)medici.size() == 0) {
            cout << "Nu exista medici!" << endl;
            return;
        }
        char nume[256];
        cout << "Numele medicului: ";
        cin >> ws;
        cin.getline(nume, 256);
        Medic* m = gasesteMedicDupaNume(nume);
        if (!m) {
            cout << "Medic inexistent: " << nume << endl;
            return;
        }
        cout << "Salariu actual: " << m->salariu << " RON\n Salariu nou: ";
        float s;
        cin >> s;
        m->salariu = s;
        cout << "Salariu actualizat!" << endl;
    }

    static Medic* adaugaMedicMeniu() {
        Medic* m = new Medic();
        cin >> *m;
        adaugaMedic(m);
        Jurnal::getInstance().log("Adaugat medic: " + m->getNume());
        cout << "Medic adaugat (id=" << m->idMedic << ")" << endl;
        return m;
    }

    static void stergeMedicDupaId() {
        if (medici.empty()) {
            cout << "Nu exista medici!" << endl;
            return;
        }
        int id;
        cout << "Id-ul medicului: ";
        cin >> id;
        auto it = find_if(medici.begin(), medici.end(),
                          [id](Medic* m){ return m->getIdMedic() == id; });
        if (it == medici.end()) {
            cout << "Inexistent!" << endl;
            return;
        }
        delete *it;
        medici.erase(it);
        cout << "Medic sters!" << endl;
    }
};
int Medic::nrMedici = 0;
vector<Medic*> Medic::medici;

ostream& operator<<(ostream& out, const Medic& m) {
    out << "---- Medic ID=" << m.idMedic << " ----" << endl;
    out << "  Nume: " << (m.nume.empty() ? "N/A" : m.nume) << endl;
    out << "  Varsta: " << m.varsta << endl;
    out << "  Specializare: " << m.specializare << endl;
    out << "  Salariu: " << m.salariu << " RON" << endl;
    out << "  Programari: " << m.nrProgramari << endl;
    out << "  Activ: " << (m.activ ? "Da" : "Nu") << endl;
    out << "------------------------" << endl;
    return out;
}

istream& operator>>(istream& in, Medic& m) {
    char buf[256];
    cout << "Numele medicului: ";
    in >> ws;
    in.getline(buf, 256);
    m.setNume(buf);

    cout << "Varsta: ";
    in >> m.varsta;

    char specBuf[16];
    do {
        cout << "Specializarea (1 caracter, ex: C/N/P): ";
        in >> ws;
        in.getline(specBuf, 16);
        if (string(specBuf).length() != 1)
            cout << "Trebuie un singur caracter!" << endl;
    } while (string(specBuf).length() != 1);
    m.specializare = specBuf[0];

    cout << "Salariul: "; in >> m.salariu;
    cout << "Activ (1=Da, 0=Nu): "; in >> m.activ;
    m.nrProgramari = 0;
    return in;
}


class MedicChirurg final : public Medic {
private:
    int numarOperatii;
    string sectie;
public:
    // constructor fara parametrii
    MedicChirurg() : Medic(), numarOperatii(0) {
        sectie = "";
        cout << "S-a creat medic chirurg cu id=" << getIdMedic() << endl;
    }

    //constructor cu toti parametrii
    MedicChirurg(const string& nume, int varsta, char specializare, float salariu, bool activ, int numarOperatii, const string& sectie) : Medic(nume, varsta, specializare, salariu, activ), numarOperatii(numarOperatii) {
        if (!sectie.empty()) {
            this->sectie = sectie;
        }
        else {
            this->sectie = "";
        }
        cout << "S-a creat medic chirug cu id=" << getIdMedic() << endl;
    }

    // copy constructor
    MedicChirurg(const MedicChirurg& mc) : Medic(mc), numarOperatii(mc.numarOperatii) {
        if (!mc.sectie.empty()) {
            sectie = mc.sectie;
        }
        else {
            sectie = "";
        }
    }

    ~MedicChirurg() override {
        cout << "S-a eliminat medic chirug cu id=" << getIdMedic() << endl;
    }

    // suprascriere operator =
    MedicChirurg& operator=(const MedicChirurg& mc) {
        if (this == &mc)
            return *this;
        Medic::operator=(mc);
        numarOperatii = mc.numarOperatii;
        sectie = mc.sectie.empty() ? "" : mc.sectie;
        return *this;
    }

    // suprascrierea unei metode a bazei + apeleaza versiunea bazei
    void afiseaza() const override {
        Medic::afiseaza();
        cout << "  [Chirurg] Sectie: " << (sectie.empty() ? "N/A" : sectie)
             << " | Operatii: " << numarOperatii << endl;
    }

    string toString() const override {
        ostringstream oss;
        oss << "MedicChirurg[id=" << getId() << ", nume=" << nume
            << ", sectie=" << sectie << ", operatii=" << numarOperatii << "]";
        return oss.str();
    }

    //getteri
    const char* getRol() const override {
        return "MedicChirurg";
    }

    int getNumarOperatii() const {
        return numarOperatii;
    }
    const string& getSectie() const {
        return sectie;
    }
    void setNumarOperatii(int n) {
        numarOperatii = n;
    }
    void incrementOperatii() {
        numarOperatii++;
    }

    //stter
    void setSectie(const string& s) {
        sectie = s;
    }

    // operator>>
    friend istream& operator>>(istream& in, MedicChirurg& mc);

    //functie meniu
};

istream& operator>>(istream& in, MedicChirurg& mc) {
    char buf[256];
    cout << "Numele medicului: ";
    in >> ws;
    in.getline(buf, 256);
    mc.setNume(buf);
    cout << "Varsta: ";
    in >> mc.varsta;
    char specBuf[16];
    do {
        cout << "Specializarea (1 caracter, ex: C/N/P): ";
        in >> ws;
        in.getline(specBuf, 16);
        if (string(specBuf).length() != 1)
            cout << "Trebuie un singur caracter!" << endl;
    } while (string(specBuf).length() != 1);
    mc.setSpecializare(specBuf[0]);

    float salariu; cout << "Salariul: ";
    in >> salariu;
    mc.setSalariu(salariu);
    int activ;
    cout << "Activ (1=Da, 0=Nu): ";
    in >> activ;
    mc.setActiv(activ != 0);

    cout << "Sectie: ";
    in >> ws;
    in.getline(buf, 256);
    mc.setSectie(buf);

    int op; cout << "Numar operatii efectuate: ";
    in >> op;
    mc.setNumarOperatii(op);
    return in;
}


class Pacient : public Persoana {
private:
    string cnp;
    bool asigurare;
    vector<float> valoriMedicale;

    static const int CNP_LEN = 13;

    static int nrPacienti;
    static vector<Pacient*> pacienti;  // Ales pentru iterare eficienta in O(N).

public:
    // constructor fara parametri
    Pacient() : Persoana(), asigurare(false) {
        cnp = "";
        cout << "S-a creat pacientul" << endl;
    }

    // constructor cu toti parametrii
    Pacient(const string& cnp, const string& nume, int varsta, bool asigurare, const vector<float>& valoriMedicale) : Persoana(nume, varsta), asigurare(asigurare), valoriMedicale(valoriMedicale) {
        this->cnp = cnp.empty() ? "" : cnp;
        cout << "S-a creat pacientul cu cnp=" << this->cnp << endl;
        Jurnal::getInstance().log("Adaugat pacient: " + nume + " cnp=" + this->cnp);
        adaugaPacient(this);
    }

    // constructor cu 3 parametri
    Pacient(const string& cnp, const string& nume, int varsta) : Persoana(nume, varsta), asigurare(false) {
        this->cnp = cnp.empty() ? "" : cnp;
        cout << "S-a creat pacientul cu cnp=" << this->cnp << endl;
        adaugaPacient(this);
    }

    // constructor cu 2 parametri
    Pacient(const string& cnp, const string& nume) : Persoana(nume, 0), asigurare(false) {
        this->cnp = cnp.empty() ? "" : cnp;
        cout << "S-a creat pacientul cu cnp=" << this->cnp << endl;
        adaugaPacient(this);
    }

    // copy constructor
    Pacient(const Pacient& p) : Persoana(p), asigurare(p.asigurare), valoriMedicale(p.valoriMedicale) {
        cnp = p.cnp.empty() ? "" : p.cnp;
    }

    // destructor (override)
    ~Pacient() override {
        cout << "S-a eliminat pacientul cu cnp=" << (cnp.empty() ? "N/A" : cnp) << endl;
    }

    // suprascrierea operatorului =
    Pacient& operator=(const Pacient& p) {
        if (this == &p) return *this;
        Persoana::operator=(p);
        asigurare = p.asigurare;
        cnp = p.cnp.empty() ? "" : p.cnp;
        valoriMedicale = p.valoriMedicale;
        return *this;
    }

    // supracrierea operatorului []
    float operator[](int index) const {
        if (index >= 0 && index < (int)valoriMedicale.size()) return valoriMedicale[index];
        cout << "Index invalid! Pacientul are " << valoriMedicale.size() << " valori." << endl;
        return 0.0f;
    }

    // operatori de citire/afisare
    friend ostream& operator<<(ostream& out, const Pacient& p);
    friend istream& operator>>(istream& in, Pacient& p);

    string toString() const override {
        ostringstream oss;
        oss << "Pacient[id=" << getId() << ", cnp=" << cnp << ", nume=" << nume << ", varsta=" << varsta << "]";
        return oss.str();
    }

    //doi pacienti sunt egali daca au acelasi cnp
    bool operator==(const Pacient& p) const {
        return !p.cnp.empty() && cnp == p.cnp;
    }

    //coparam pacientii din punct de vedere al varstei
    bool operator<=(const Pacient& p) const {
        return varsta <= p.varsta;
    }

    //modificam asigurarea pacientului : ++ pentru a avea asigurare, -- pentru a nu mai avea asigurare
    Pacient& operator++() {
        asigurare = true;
        return *this;
    }
    Pacient operator++(int) {
        Pacient t(*this);
        asigurare = true;
        return t;
    }
    Pacient& operator--() {
        asigurare = false;
        return *this;
    }

    //daca adaugam un float la un pacient, adaugam inca o valoarea in vectorul de valori medicale inregistrate
    Pacient operator+(float valoare) const {
        Pacient rez(*this);
        rez.valoriMedicale.push_back(valoare);
        return rez;
    }
    // comutativ
    friend Pacient operator+(float v, const Pacient& p) {
        return p + v;
    }

    //daca scadem un float de la un pacient si aceasta se afla in vectorul de valori medicale, o stergem din vector
    Pacient operator-(float valoare) const {
        Pacient rez(*this);
        auto it = find(rez.valoriMedicale.begin(), rez.valoriMedicale.end(), valoare);
        if (it != rez.valoriMedicale.end()) rez.valoriMedicale.erase(it);
        return rez;
    }
    //comutativ
    friend Pacient operator-(float v, const Pacient& p) {
        return p - v;
    }

    // override functii virtuale
    void afiseaza() const override {
        cout << "PACIENT CNP=" << (cnp.empty() ? "N/A" : cnp) << " | ";
        afiseazaInfoComuna();
        cout << " | Asigurat=" << (asigurare ? "Da" : "Nu")
             << " | NrValori=" << valoriMedicale.size() << endl;
    }
    const char* getRol() const override {
        return "Pacient";
    }

    // getteri
    const string& getCnp() const {
        return cnp;
    }
    bool getAsigurare() const {
        return asigurare;
    }
    const vector<float>& getValoriMedicale() const {
        return valoriMedicale;
    }
    int getNrValori() const {
        return (int)valoriMedicale.size();
    }
    static int getNrPacienti() {
        return (int)pacienti.size();
    }
    static const vector<Pacient*>& getPacienti() {
        return pacienti;
    }

    // setteri
    void setAsigurare(bool a) {
        asigurare = a;
    }
    void setCnp(const string& c) {

        if (!c.empty()) {
            cnp = c;
        } else {
            cnp = new char[1];
            cnp[0] = '\0';
        }
    }

    // adauga pacienti in vector
    static void adaugaPacient(Pacient* p) {
        if (p == nullptr) throw ClinicaException("Pacient nullptr nu se poate adauga");
        pacienti.push_back(p);
    }

    // sterge pacientii din vector
    static void stergePacienti() {
        for (Pacient* p : pacienti) delete p;
        pacienti.clear();
        nrPacienti = 0;
    }

    // validare cnp
    static void valideazaCnp(const string& cnp) {
        if (cnp.length() != 13) {
            throw ValidareException("CNP", cnp, "trebuie 13 cifre");
        }
        for (int i = 0; i < 13; i++) {
            if (!isdigit((unsigned char)cnp[i])) {
                throw ValidareException("CNP", cnp, "doar cifre permise");
            }
        }
    }

    static bool esteValidCnp(const string& cnp) noexcept {
        try { valideazaCnp(cnp); return true; }
        catch (...) { return false; }
    }
    static bool cnpExistent(const string& cnp) noexcept {
        return any_of(pacienti.begin(), pacienti.end(),
                      [&cnp](Pacient* p){ return p->getCnp() == cnp; });
    }


    static Pacient* gasesteDupaCnp(const string& cnp) {
        auto it = find_if(pacienti.begin(), pacienti.end(),
                          [&cnp](Pacient* p){ return p->getCnp() == cnp; });
        if (it == pacienti.end())
            throw EntitateInexistentaException("Pacient", "cnp=" + cnp);
        return *it;
    }

    static Pacient* gasesteDupaNume(const string& nume) {
        auto it = find_if(pacienti.begin(), pacienti.end(),
                          [&nume](Pacient* p){ return p->getNume() == nume; });
        return (it != pacienti.end()) ? *it : nullptr;
    }

    // functii meniu
    static void afiseazaPacienti() {
        if (pacienti.empty()) {
            cout << "Nu exista pacienti!" << endl;
            return;
        }
        for_each(pacienti.begin(), pacienti.end(), [](Pacient* p){ cout << *p << endl; });
    }

    static void afiseazaValoriMedicale() {
        char cnpBuf[32]; cout << "CNP-ul pacientului: "; cin >> cnpBuf;
        Pacient* p = gasesteDupaCnp(cnpBuf);
        if (!p) { cout << "Inexistent!" << endl; return; }
        if (p->getNrValori() == 0) { cout << "Fara valori inregistrate." << endl; return; }
        cout << "Valori: ";
        for (float v : p->getValoriMedicale()) cout << v << " ";
        cout << endl;
    }

    static Pacient* adaugaPacientMeniu() {
        Pacient* p = new Pacient();
        cin >> *p;
        adaugaPacient(p);
        Jurnal::getInstance().log("Adaugat pacient: " + p->getNume() + " cnp=" + p->getCnp());
        cout << "Pacient adaugat (cnp=" << p->getCnp() << ")" << endl;
        return p;
    }

    static void modificaPacient() {
        if ((int)pacienti.size() == 0) {
            cout << "Nu exista pacienti!" << endl;
            return;
        }
        char cnpBuf[32];
        cout << "CNP-ul: ";
        cin >> cnpBuf;
        Pacient* p = gasesteDupaCnp(cnpBuf);
        if (!p) {
            cout << "Inexistent!" << endl;
            return;
        }
        int opt;
        cout << "1.Varsta 2.Asigurare\nOptiune: ";
        cin >> opt;
        if (opt == 1) {
            int v;
            cout << "Varsta: ";
            cin >> v;
            p->setVarsta(v);
        }
        else if (opt == 2) {
            int a;
            cout << "Asigurare(1/0): ";
            cin >> a;
            p->asigurare = (a != 0);
        }
        cout << "Actualizat!" << endl;
    }

    static void stergePacient() {
        if (pacienti.empty()) {
            cout << "Nu exista pacienti!" << endl;
            return;
        }
        string cnpBuf;
        cout << "CNP-ul pacientului: ";
        cin >> cnpBuf;
        auto it = find_if(pacienti.begin(), pacienti.end(),
                          [&cnpBuf](Pacient* p){ return p->getCnp() == cnpBuf; });
        if (it == pacienti.end()) {
            cout << "Pacient inexistent!" << endl;
            return;
        }
        delete *it;
        pacienti.erase(it);
        cout << "Pacient sters!" << endl;
    }

};
int Pacient::nrPacienti = 0;
vector<Pacient*> Pacient::pacienti;

ostream& operator<<(ostream& out, const Pacient& p) {
    out << "---- Pacient CNP=" << (p.getCnp().empty() ? "N/A" : p.getCnp()) << " ----" << endl;
    out << "  Nume: " << (p.getNume().empty() ? "N/A" : p.getNume()) << endl;
    out << "  Varsta: " << p.getVarsta() << endl;
    out << "  Asigurare: " << (p.getAsigurare() ? "Da" : "Nu") << endl;
    out << "  Valori medicale: ";
    if (p.getNrValori() > 0) {
        for (float v : p.getValoriMedicale()) out << v << " ";
    } else out << "(niciuna)";
    out << endl << "------------------------" << endl;
    return out;
}
istream& operator>>(istream& in, Pacient& p) {
    char buf[256]; cout << "Numele pacientului: ";
    in >> ws;
    in.getline(buf, 256); p.setNume(buf);
    char cnpBuf[32];
    bool ok = false;
    do {
        cout << "CNP (13 cifre): "; in >> cnpBuf;
        try { Pacient::valideazaCnp(cnpBuf); }
        catch (const ValidareException& e) { cout << e.what() << endl; continue; }
        if (Pacient::cnpExistent(cnpBuf)) { cout << "CNP deja existent!" << endl; continue; }
        ok = true;
    }
    while (!ok);
    p.setCnp(cnpBuf);

    cout << "Varsta: ";
    in >> p.varsta;
    int a; cout << "Asigurare(1=Da,0=Nu): ";
    in >> a;
    p.setAsigurare(a != 0);

    int n; cout << "Numar valori medicale (>=0): ";
    in >> n;
    if (n < 0) n = 0;
    for (int i = 0; i < n; i++) {
        float val; cout << "Valoarea " << (i + 1) << ": "; in >> val;
        p = p + val;
    }
    return in;
}

class PacientCronic : public Pacient {
private:
    string afectiune;
    int gradGravitate;    // 1-5
public:
    // constructor fara parametrii
    PacientCronic() : Pacient(), gradGravitate(1) {
        afectiune = "";
        cout << "S-a creat pacient cronic" << endl;
    }

    // constructir cu toti parametrii
    PacientCronic(const string& cnp, const string& nume, int varsta, bool asigurare, const vector<float>& valoriMedicale, const string& afectiune, int gradGravitate)
        : Pacient(cnp, nume, varsta, asigurare, valoriMedicale), gradGravitate(gradGravitate) {
        this->afectiune = afectiune.empty() ? "" : afectiune;
        cout << "S-a creat pacient cronic cu cnp=" << getCnp() << endl;
    }

    // copy constructor
    PacientCronic(const PacientCronic& pc) : Pacient(pc), gradGravitate(pc.gradGravitate) {
        afectiune = pc.afectiune.empty() ? "" : pc.afectiune;
    }

    // destructor
    ~PacientCronic() override {
        cout << "S-a eliminat pacient cronic cu cnp=" << getCnp() << endl;
    }

    //suprascrierea operatorului =
    PacientCronic& operator=(const PacientCronic& pc) {
        if (this == &pc)
            return *this;
        Pacient::operator=(pc);
        gradGravitate = pc.gradGravitate;
        afectiune = pc.afectiune.empty() ? "" : pc.afectiune;
        return *this;
    }

    //suprascriere functie mostenire
    void afiseaza() const override {
        Pacient::afiseaza();
        cout << "  [Cronic] Afectiune: " << (afectiune.empty() ? "N/A" : afectiune)
             << " | Grad: " << gradGravitate << "/5" << endl;
    }

    string toString() const override {
        ostringstream oss;
        oss << "PacientCronic[id=" << getId() << ", cnp=" << getCnp()
            << ", afectiune=" << afectiune << ", grad=" << gradGravitate << "]";
        return oss.str();
    }

    //getteri
    const char* getRol() const override {
        return "PacientCronic";
    }

    const string& getAfectiune() const {
        return afectiune;
    }
    int getGradGravitate() const {
        return gradGravitate;
    }

    // setteri
    void setGradGravitate(int g) {
        gradGravitate = g;
    }

    void setAfectiune(const string& a) {
        afectiune = a;
    }

    // operator>>
    friend istream& operator>>(istream& in, PacientCronic& pc);

};

istream& operator>>(istream& in, PacientCronic& pc) {
    char buf[256]; cout << "Numele pacientului: "; in >> ws; in.getline(buf, 256); pc.setNume(buf);
    char cnpBuf[32]; bool ok = false;
    do {
        cout << "CNP (13 cifre): "; in >> cnpBuf;
        if (!Pacient::esteValidCnp(cnpBuf)) continue;
        if (Pacient::cnpExistent(cnpBuf)) { cout << "CNP deja existent!" << endl; continue; }
        ok = true;
    } while (!ok);
    pc.setCnp(cnpBuf);

    int v; cout << "Varsta: "; in >> v; pc.setVarsta(v);
    int a; cout << "Asigurare(1/0): "; in >> a; bool asigurat = (a != 0);

    int nrVal; cout << "Numar valori medicale (>=0): "; in >> nrVal;
    vector<float> valori;
    if (nrVal > 0) {
        for (int i = 0; i < nrVal; i++) {
            float val; cout << "Valoarea " << (i + 1) << ": "; in >> val;
            valori.push_back(val);
        }
    }
    Pacient temp(pc.getCnp(), pc.getNume(), pc.getVarsta(), asigurat, valori);
    static_cast<Pacient&>(pc) = temp;

    cout << "Afectiune cronica: "; in >> ws; in.getline(buf, 256); pc.setAfectiune(buf);
    int g; cout << "Grad gravitate (1-5): "; in >> g;
    if (g < 1) g = 1; if (g > 5) g = 5;
    pc.setGradGravitate(g);
    return in;
}


class Programare : public IdObiect {
private:
    int idProgramare;
    int idMedicReal;
    int idPacientNum;
    string data;           // dd-mm-yyyy
    string ora;             // hh:mm

    Medic* medicRef;         // pointer la medicul real
    Pacient* pacientRef;     // pointer la pacientul real

    static int nextId;
    static list<Programare*> programari;

public:
    //constructor fara parametri
    Programare(): idProgramare(++nextId), idMedicReal(0), idPacientNum(0), medicRef(nullptr), pacientRef(nullptr) {
        data = "00-00-0000";
        ora = "00:00";
        cout << "S-a creat programarea cu id-ul: " << idProgramare << endl;
    }

    //constructor cu toti parametrii
    Programare(const string& data, const string& ora, Medic* m, Pacient* p): idProgramare(++nextId),idMedicReal(m ? m->getIdMedic() : 0),idPacientNum(0),medicRef(m), pacientRef(p) {
        this->data = data;
        this->ora = ora;
        cout << "S-a creat programarea cu id-ul: " << idProgramare << endl;
        Jurnal::getInstance().log("Programare noua: " + data + " " + ora);
    }

    // constructor cu 2 parametrii
    Programare(Medic* m, Pacient* p): idProgramare(++nextId),idMedicReal(m ? m->getIdMedic() : 0),idPacientNum(0), medicRef(m), pacientRef(p) {
        data = "00-00-0000";
        ora = "00:00";
        cout << "S-a creat programarea cu id-ul: " << idProgramare << endl;
    }

    // constructor cu 3 parametrii
    Programare(Medic* m, Pacient* p, const string& data): idProgramare(++nextId), idMedicReal(m ? m->getIdMedic() : 0), idPacientNum(0), medicRef(m), pacientRef(p) {
        this->data = data;
        ora = "00:00";
        cout << "S-a creat programarea cu id-ul: " << idProgramare << endl;
    }

    //copy constructor
    Programare(const Programare& p): idProgramare(p.idProgramare), idMedicReal(p.idMedicReal), idPacientNum(p.idPacientNum), medicRef(p.medicRef), pacientRef(p.pacientRef) {
        data = p.data;
        ora = p.ora;
    }

    ~Programare() {
        cout << "Am sters programarea cu id-ul: " << idProgramare << " prin destructor"<<endl;
    }

    // supracrierea operatorului =
    Programare& operator=(const Programare& p) {
        if (this == &p)
            return *this;
        idProgramare = p.idProgramare;
        idMedicReal = p.idMedicReal;
        idPacientNum = p.idPacientNum;
        data = p.data;
        ora = p.ora;
        medicRef = p.medicRef;
        pacientRef = p.pacientRef;
        return *this;
    }

    // suprascrierea operatorului [] - 0=data, 1=ora
    string operator[](int idx) const {
        if (idx == 0)
            return data;
        if (idx == 1)
            return ora;
        cout << "Indexul " << idx << " este invalid pentru programare!" << endl;
        return nullptr;
    }

    //operatorii de citire afisare
    friend ostream& operator<<(ostream& out, const Programare& p);
    friend istream& operator>>(istream& in, Programare& p);

    string toString() const override {
        ostringstream oss;
        oss << "Programare[id=" << getId() << ", data=" << data << ", ora=" << ora << "]";
        return oss.str();
    }

    // doua programari sunt egale daca au acelasi id
    bool operator==(const Programare& p) const {
        return idProgramare == p.idProgramare;
    }

    //comparam programarile din punct de vedere al datei
    bool operator<(const Programare& p) const {
        int an1 = (data[6]-'0')*1000 + (data[7]-'0')*100 + (data[8]-'0')*10 + (data[9]-'0');
        int an2 = (p.data[6]-'0')*1000 + (p.data[7]-'0')*100 + (p.data[8]-'0')*10 + (p.data[9]-'0');
        if (an1 != an2)
            return an1 < an2;

        int l1 = (data[3]-'0')*10 + (data[4]-'0');
        int l2 = (p.data[3]-'0')*10 + (p.data[4]-'0');
        if (l1 != l2)
            return l1 < l2;

        int z1 = (data[0]-'0')*10 + (data[1]-'0');
        int z2 = (p.data[0]-'0')*10 + (p.data[1]-'0');
        return z1 < z2;
    }


    // operator ++ - se adauga o ora
    Programare& operator++() {
        int h = (ora[0] - '0') * 10 + (ora[1] - '0');
        h = (h + 1) % 24;
        ora[0] = (h / 10) + '0'; ora[1] = (h % 10) + '0';
        return *this;
    }
    //operator -- -  se scade o ora
    Programare& operator--() {
        int h = (ora[0] - '0') * 10 + (ora[1] - '0');
        h = (h - 1 + 24) % 24;
        ora[0] = (h / 10) + '0'; ora[1] = (h % 10) + '0';
        return *this;
    }

    //daca adunam o programare cu un int, adaugam cantitatea la ora programarii
    Programare operator+(int c) const {
        Programare rez(*this);
        int h = (rez.ora[0] - '0') * 10 + (rez.ora[1] - '0');
        h = ((h + c) % 24 + 24) % 24;
        rez.ora[0] = (h / 10) + '0';
        rez.ora[1] = (h % 10) + '0';
        return rez;
    }

    //daca scadem o programare cu un int, scadem din ora programarii cantitatea
    Programare operator-(int c) const {
        return (*this) + (-c);
    }

    // getteri
    int getIdProgramare() const {
        return idProgramare;
    }
    int getIdMedicReal() const {
        return idMedicReal;
    }
    const string& getData() const {
        return data;
    }
    const string& getOra() const {
        return ora;
    }
    Medic* getMedic() const {
        return medicRef;
    }
    Pacient* getPacient() const {
        return pacientRef;
    }
    static int getNrProgramari() { return (int)programari.size(); }
    static const list<Programare*>& getProgramari() { return programari; }

    // setteri
    void setData(const string& d) {
        data = d;
    }
    void setOra(const string& o) {
        ora = o;
    }
    void setMedic(Medic* m) {
        medicRef = m;
        idMedicReal = m ? m->getIdMedic() : 0;
    }
    void setPacient(Pacient* p) {
        pacientRef = p;
    }

    // adauga programari in vector
    static void adaugaProgramare(Programare* p) {
        if (p == nullptr) throw ClinicaException("Programare nullptr nu se poate adauga");
        programari.push_back(p);
    }

    // sterge programarile din vector
    static void stergeProgramari() {
        for (Programare* p : programari) delete p;
        programari.clear();
    }

    // validare data
    static void valideazaData(const string& data) {
        if (data.length() != 10 || data[2] != '-' || data[5] != '-' || !isdigit(data[0]) || !isdigit(data[1]) || !isdigit(data[3]) || !isdigit(data[4]) || !isdigit(data[6]) || !isdigit(data[7]) || !isdigit(data[8]) || !isdigit(data[9])) {
            throw ValidareException("Data", data, "format invalid (dd-mm-yyyy)");
        }

        int zi = (data[0] - '0') * 10 + (data[1] - '0');
        int luna = (data[3] - '0') * 10 + (data[4] - '0');
        int an = (data[6] - '0') * 1000 + (data[7] - '0') * 100 + (data[8] - '0') * 10 + (data[9] - '0');

        if (luna < 1 || luna > 12) {
            if (luna < 1 || luna > 12) throw ValidareException("Data", data, "luna invalida");
        }

        if (zi < 1 || zi > 31) {
            throw ValidareException("Data", data, "zi invalida");
        }

        if ((luna == 4 || luna == 6 || luna == 9 || luna == 11) && zi > 30) {
            throw ValidareException("Data", data, "luna are doar 30 de zile");
        }

        if (luna == 2) {
            if (an % 4 == 0 && zi > 29) {
                throw ValidareException("Data", data, "februarie are 29 zile");
            }

            else if (an % 4 != 0 && zi > 28) {
                throw ValidareException("Data", data, "februarie are 28 zile");
            }
        }

        if (an < 2026) {
            throw ValidareException("Data", data, "data este in trecut");
        }

    }

    static bool esteValidData(const string& data) noexcept {
        try { valideazaData(data); return true; }
        catch (...) { return false; }
    }

    // validare ora
    static void valideazaOra(const string& ora) {
        if (ora.length() != 5 || ora[2] != ':' || !isdigit(ora[0]) || !isdigit(ora[1]) || !isdigit(ora[3]) || !isdigit(ora[4])) {
            throw ValidareException("Ora", ora, "format invalid (hh:mm)");
        }

        int h = (ora[0] - '0') * 10 + (ora[1] - '0');
        int m = (ora[3] - '0') * 10 + (ora[4] - '0');

        if (h < 0 || h > 23) {
            throw ValidareException("Ora", ora, "ora invalida (0-23)");
        }

        if (m < 0 || m > 59) {
            throw ValidareException("Ora", ora, "minute invalide (0-59)");
        }
    }

    static bool esteValidOra(const string& ora) noexcept {
        try { valideazaOra(ora); return true; }
        catch (...) { return false; }
    }

    // functii pentru meniu
    static void afiseazaProgramariMedic() {
        if (programari.empty()) { cout << "Nu exista programari!" << endl; return; }
        int id; cout << "Id-ul medicului: "; cin >> id;
        bool gasit = false;
        for_each(programari.begin(), programari.end(),
                 [id, &gasit](Programare* p){
                     if (p->getIdMedicReal() == id) { cout << *p; gasit = true; }
                 });
        if (!gasit) cout << "Nicio programare pentru id=" << id << endl;
    }

    static void afiseazaProgramariPacient() {
        if (programari.empty()) { cout << "Nu exista programari!" << endl; return; }
        string cnpBuf; cout << "CNP pacient: "; cin >> cnpBuf;
        bool gasit = false;
        for_each(programari.begin(), programari.end(),
                 [&cnpBuf, &gasit](Programare* p){
                     Pacient* pp = p->getPacient();
                     if (pp && pp->getCnp() == cnpBuf) { cout << *p; gasit = true; }
                 });
        if (!gasit) cout << "Nicio programare pentru cnp=" << cnpBuf << endl;
    }

    static void createProgramare() {
        Programare* p = new Programare();
        cin >> *p;
        adaugaProgramare(p);
        cout << "Programare inregistrata id=" << p->idProgramare << endl;
    }

    static void modificaProgramare() {
        if (programari.empty()) { cout << "Nu exista programari!" << endl; return; }
        int id; cout << "Id programare: "; cin >> id;
        auto it = find_if(programari.begin(), programari.end(),
                          [id](Programare* p){ return p->getIdProgramare() == id; });
        if (it == programari.end()) { cout << "Inexistenta!" << endl; return; }
        string d, o;
        cout << "Noua data dd-mm-yyyy: "; cin >> d;
        while (!esteValidData(d)) { cout << "Re-introdu data: "; cin >> d; }
        cout << "Noua ora hh:mm: "; cin >> o;
        while (!esteValidOra(o)) { cout << "Re-introdu ora: "; cin >> o; }
        (*it)->setData(d); (*it)->setOra(o);
        cout << "Modificata!" << endl;
    }

    static void stergeProgramare() {
        if (programari.empty()) { cout << "Nu exista programari!" << endl; return; }
        int id; cout << "Id programare: "; cin >> id;
        // Cautam mai intai sa nu pierdem pointerul - apoi remove_if membru list
        Programare* deSters = nullptr;
        for (Programare* p : programari)
            if (p->getIdProgramare() == id) { deSters = p; break; }
        if (deSters == nullptr) { cout << "Inexistenta!" << endl; return; }
        if (deSters->getMedic()) deSters->getMedic()->decrementeazaProgramari();
        delete deSters;
        // remove_if membru pentru list - cerinta 3 din proiect
        programari.remove_if([id](Programare* p){ return p == nullptr || p->getIdProgramare() == id; });
        cout << "Stearsa!" << endl;
    }
};
int Programare::nextId = 0;
list<Programare*> Programare::programari;

ostream& operator<<(ostream& out, const Programare& p) {
    out << "---- Programare ID=" << p.idProgramare << " ----" << endl;
    out << "  Data: " << p.data << "  Ora: " << p.ora << endl;
    out << "  Medic: ";
    if (p.medicRef) out << p.medicRef->getNume() << " (id=" << p.medicRef->getIdMedic() << ")";
    else out << "N/A";
    out << endl << "  Pacient: ";
    if (p.pacientRef) out << p.pacientRef->getNume() << " (cnp=" << p.pacientRef->getCnp() << ")";
    else out << "N/A";
    out << endl << "------------------------" << endl;
    return out;
}

istream& operator>>(istream& in, Programare& pr) {
    char d[16], o[16];
    cout << "Data dd-mm-yyyy: ";
    in >> d;
    while (!Programare::esteValidData(d)) {
        cout << "Data invalida! Re-introdu: ";
        in >> d;
    }
    pr.setData(d);
    cout << "Ora hh:mm: ";
    in >> o;
    while (!Programare::esteValidOra(o)) {
        cout << "Ora invalida! Re-introdu: ";
        in >> o;
    }
    pr.setOra(o);

    int idMed; cout << "Id-ul medicului: ";
    in >> idMed;
    Medic* m = Medic::gasesteMedicDupaId(idMed);
    while (!m) {
        cout << "Medic inexistent! Re-introdu id: ";
        in >> idMed;
        m = Medic::gasesteMedicDupaId(idMed);
    }
    pr.setMedic(m);
    m->incrementeazaProgramari();

    char cnpBuf[32];
    cout << "CNP-ul pacientului: ";
    in >> cnpBuf;
    Pacient* pp = Pacient::gasesteDupaCnp(cnpBuf);
    while (!pp) {
        cout << "Pacient inexistent! Re-introdu cnp: ";
        in >> cnpBuf;
        pp = Pacient::gasesteDupaCnp(cnpBuf);
    }
    pr.setPacient(pp);
    return in;
}

class Reteta : public IdObiect {
private:
    long long idReteta;

    Programare programare;
    string medicamente[10];

    float pretMedicamente[10];
    int nrMedicamente;
    float pretTotal;

    static unordered_map<long long, Reteta*> retete;
    static long long nextId;

public:
    // constructor fara parametrii
    Reteta(): idReteta(++nextId), programare(), nrMedicamente(0), pretTotal(0.0f) {
        for (int i = 0; i < 10; i++) {
            medicamente[i] = "";
            pretMedicamente[i] = 0.0f;
        }
        cout << "S-a creat reteta cu id = " << idReteta << endl;
    }
    // constructor cu toti parametrii
    Reteta(const Programare& prog, const string meds[], const float preturi[], int nr): idReteta(++nextId), programare(prog), nrMedicamente(nr <= 10 ? nr : 10), pretTotal(0.0f) {
        if (nr > 10)
            cout << "Limitat la 10 medicamente." << endl;
        for (int i = 0; i < 10; i++) {
            medicamente[i] = "";
            pretMedicamente[i] = 0.0f;
        }
        for (int i = 0; i < nrMedicamente; i++) {
            medicamente[i] = meds[i];
            pretMedicamente[i] = preturi[i];
            pretTotal += preturi[i];
        }
        if (programare.getPacient() && programare.getPacient()->getAsigurare())
            pretTotal *= 0.1f;
        cout << "S-a creat reteta cu id = " << idReteta << endl;
    }

    // constructor cu 3 parametrii
    Reteta(const Programare& prog, const string meds[], int nr): idReteta(++nextId), programare(prog), nrMedicamente(nr <= 10 ? nr : 10), pretTotal(0.0f) {
        for (int i = 0; i < 10; i++) {
            medicamente[i] = "";
            pretMedicamente[i] = 0.0f;
        }
        for (int i = 0; i < nrMedicamente; i++) {
            medicamente[i] = meds[i];
            pretMedicamente[i] = 1.5f;
        }
        pretTotal = 1.5f * nrMedicamente;
        if (programare.getPacient() && programare.getPacient()->getAsigurare())
            pretTotal *= 0.1f;
        cout << "S-a creat reteta cu id = " << idReteta << endl;
    }

    // copy constructor
    Reteta(const Reteta& r): idReteta(r.idReteta), programare(r.programare), nrMedicamente(r.nrMedicamente), pretTotal(r.pretTotal) {
        for (int i = 0; i < 10; i++) {
            medicamente[i] = r.medicamente[i];
            pretMedicamente[i] = r.pretMedicamente[i];
        }
    }

    // desctructor
    ~Reteta() {
        cout << "S-a sters reteta cu id = " << idReteta << endl;
    }

    // suprascrierea operatorului egal
    Reteta& operator=(const Reteta& r) {
        if (this == &r)
            return *this;
        idReteta = r.idReteta;
        programare = r.programare;
        nrMedicamente = r.nrMedicamente;
        pretTotal = r.pretTotal;
        for (int i = 0; i < 10; i++) {
            medicamente[i] = r.medicamente[i];
            pretMedicamente[i] = r.pretMedicamente[i];
        }
        return *this;
    }

    // suprascrierea operatorului de indexare
    string operator[](int idx) const {
        if (idx >= 0 && idx < nrMedicamente)
            return medicamente[idx];
        cout << "Indexul " << idx << " este invalid pentru reteta aceasta";
        return "N/A";
    }

    // operatori de citire/afisare
    friend ostream& operator<<(ostream& out, const Reteta& r);
    friend istream& operator>>(istream& in, Reteta& r);

    string toString() const override {
        ostringstream oss;
        oss << "Reteta[id=" << getId() << ", total=" << pretTotal << " RON]";
        return oss.str();
    }

    // doua retete sunt egale daca au acelasi id
    bool operator==(const Reteta& r) const {
        return idReteta == r.idReteta;
    }

    //comparam doua retete din punct de vedere al pretului de achitat
    bool operator<(const Reteta& r) const {
        return pretTotal < r.pretTotal;
    }
    bool operator>(const Reteta& r) const {
        return pretTotal > r.pretTotal;
    }
    bool operator<=(const Reteta& r) const {
        return pretTotal <= r.pretTotal;
    }
    bool operator>=(const Reteta& r) const {
        return pretTotal >= r.pretTotal;
    }

    //++ creste pretul fiecarui medicament cu 1.5 si -- scade pretul
    Reteta& operator++() {
        for (int i = 0; i < nrMedicamente; i++)
            pretMedicamente[i] += 1.5f;
        recalculeazaTotal();
        return *this;
    }
    Reteta& operator--() {
        for (int i = 0; i < nrMedicamente; i++)
            pretMedicamente[i] = max(0.0f, pretMedicamente[i] - 1.5f);
        recalculeazaTotal();
        return *this;
    }

    // daca adunam o reteta cu un float, scumpim fiecare medicament cu acea cantitate
    Reteta operator+(float f) const {
        Reteta rez(*this);
        for (int i = 0; i < rez.nrMedicamente; i++)
            rez.pretMedicamente[i] += f;
        rez.recalculeazaTotal();
        return rez;
    }
    friend Reteta operator+(float f, const Reteta& r) {
        return r + f;
    }

    // daca scadem un flaot dintr-o reteta, scadem pretul cu acea cantitate
    Reteta operator-(float f) const {
        Reteta rez(*this);
        for (int i = 0; i < rez.nrMedicamente; i++)
            rez.pretMedicamente[i] = max(0.0f, rez.pretMedicamente[i] - f);
        rez.recalculeazaTotal();
        return rez;
    }
    friend Reteta operator-(float f, const Reteta& r) {
        return r - f;
    }

    // getteri
    long long getIdReteta() const {
        return idReteta;
    }
    const Programare& getProgramare() const {
        return programare;
    }
    int getNrMedicamente() const {
        return nrMedicamente;
    }
    float getPretTotal() const {
        return pretTotal;
    }
    string getMedicament(int i) const {
        return (i >= 0 && i < nrMedicamente) ? medicamente[i] : "";
    }
    float getPretMedicament(int i) const {
        return (i >= 0 && i < nrMedicamente) ? pretMedicamente[i] : 0.0f;
    }
    static int getNrRetete() { return (int)retete.size(); }
    static const unordered_map<long long, Reteta*>& getRetete() { return retete; }

    // calcularea pretului total
    void recalculeazaTotal() {
        pretTotal = 0.0f;
        for (int i = 0; i < nrMedicamente; i++)
            pretTotal += pretMedicamente[i];
        if (programare.getPacient() && programare.getPacient()->getAsigurare())
            pretTotal *= 0.1f;
    }

    void adaugaMedicament(const string& med, float pret) {
        if (nrMedicamente >= 10) {
            cout << "Limita 10!" << endl;
            return;
        }
        medicamente[nrMedicamente] = med;
        pretMedicamente[nrMedicamente] = pret;
        nrMedicamente++;
        recalculeazaTotal();
    }

    void stergeMedicament(int idx) {
        if (idx < 0 || idx >= nrMedicamente) {
            cout << "Index invalid!" << endl;
            return;
        }
        for (int i = idx; i < nrMedicamente - 1; i++) {
            medicamente[i] = medicamente[i + 1];
            pretMedicamente[i] = pretMedicamente[i + 1];
        }
        medicamente[nrMedicamente - 1] = "";
        pretMedicamente[nrMedicamente - 1] = 0.0f;
        nrMedicamente--;
        recalculeazaTotal();
    }

    //setter
    void setPretMedicament(int idx, float p) {
        if (idx >= 0 && idx < nrMedicamente) {
            pretMedicamente[idx] = p;
            recalculeazaTotal();
        }
    }

    // adauga retete in vector
    static void adaugaReteta(Reteta* r) {
        if (r == nullptr) throw ClinicaException("Reteta nullptr nu se poate adauga");
        // unordered_map - cheia este id-ul retetei
        if (retete.count(r->getIdReteta()) > 0)
            throw ClinicaException("Reteta cu id=" + to_string(r->getIdReteta()) + " exista deja");
        retete[r->getIdReteta()] = r;
    }
    // sterge retete din vector
    static void stergeRetete() {
        // Structured bindings - cerinta 3 din proiect
        for (auto& [id, r] : retete) delete r;
        retete.clear();
    }

    // verificare daca medicul exista
    static bool verificaIdMedic(int id) {
        if (Medic::getSizeMedici() == 0) {
            cout << "Nu exista medici!" << endl;
            return false;
        }
        if (Medic::gasesteMedicDupaId(id) == nullptr) {
            cout << "Medic inexistent (id=" << id << ")! Acces refuzat." << endl;
            return false;
        }
        return true;
    }

    // functii meniu
    static void afiseazaRetetePacient() {
        if (retete.empty()) { cout << "Nu exista retete!" << endl; return; }
        string cnpBuf; cout << "CNP pacient: "; cin >> cnpBuf;
        bool gasit = false;
        // structured bindings - cerinta 3
        for (const auto& [id, r] : retete) {
            Pacient* pp = r->getProgramare().getPacient();
            if (pp && pp->getCnp() == cnpBuf) { cout << *r; gasit = true; }
        }
        if (!gasit) cout << "Nicio reteta pentru cnp=" << cnpBuf << endl;
    }

    static void modificaReteta() {
        if (retete.empty()) { cout << "Nu exista retete!" << endl; return; }
        int idMed; cout << "Id-ul tau de medic: "; cin >> idMed;
        if (!verificaIdMedic(idMed)) return;
        long long id; cout << "Id reteta: "; cin >> id;
        auto it = retete.find(id);
        if (it == retete.end()) { cout << "Reteta inexistenta!" << endl; return; }
        Reteta* r = it->second;
        Medic* m = r->getProgramare().getMedic();
        if (!m || m->getIdMedic() != idMed) {
            cout << "Reteta apartine altui medic! Acces refuzat." << endl;
            return;
        }
        cout << *r;
        int opt; cout << "1.Adauga medicament 2.Sterge medicament 3.Modifica pret\nOptiune: "; cin >> opt;
        if (opt == 1) {
            string med; float pr;
            cout << "Nume medicament: "; cin >> ws; getline(cin, med);
            cout << "Pret: "; cin >> pr;
            r->adaugaMedicament(med, pr);
        } else if (opt == 2) {
            int idx; cout << "Index (0-" << r->getNrMedicamente() - 1 << "): "; cin >> idx;
            r->stergeMedicament(idx);
        } else if (opt == 3) {
            int idx; float pr;
            cout << "Index: "; cin >> idx; cout << "Pret nou: "; cin >> pr;
            r->setPretMedicament(idx, pr);
        }
        cout << *r;
    }

    static void stergeReteta() {
        if (retete.empty()) { cout << "Nu exista retete!" << endl; return; }
        int idMed; cout << "Id-ul tau de medic: "; cin >> idMed;
        if (!verificaIdMedic(idMed)) return;
        long long id; cout << "Id reteta: "; cin >> id;
        auto it = retete.find(id);
        if (it == retete.end()) { cout << "Inexistenta!" << endl; return; }
        Medic* m = it->second->getProgramare().getMedic();
        if (!m || m->getIdMedic() != idMed) {
            cout << "Reteta apartine altui medic! Acces refuzat." << endl;
            return;
        }
        delete it->second;
        retete.erase(it);
        cout << "Stearsa!" << endl;
    }

    static void adaugaRetetaMeniu() {
        if (Programare::getNrProgramari() == 0) {
            cout << "Mai intai trebuie creata o programare!" << endl;
            return;
        }
        int idMed;
        cout << "Id-ul tau de medic: ";
        cin >> idMed;
        if (!verificaIdMedic(idMed))
            return;
        Reteta* r = new Reteta();
        cin >> *r;
        Medic* m = r->programare.getMedic();
        if (!m || m->getIdMedic() != idMed) {
            cout << "Programarea selectata nu apartine de tine! Anulare." << endl;
            delete r;
            return;
        }
        adaugaReteta(r);
        cout << "Reteta creata! ID=" << r->idReteta << " | Total=" << r->pretTotal << " RON" << endl;
    }
};
unordered_map<long long, Reteta*> Reteta::retete;
long long Reteta::nextId = 0;

ostream& operator<<(ostream& out, const Reteta& r) {
    out << "---- Reteta ID=" << r.idReteta << " ----" << endl;
    out << "  Programare:" << endl << r.programare;
    out << "  Medicamente:" << endl;
    for (int i = 0; i < r.nrMedicamente; i++)
        out << "    " << (i + 1) << ". " << r.medicamente[i]
            << " - " << r.pretMedicamente[i] << " RON" << endl;
    out << "  TOTAL: " << r.pretTotal << " RON";
    if (r.programare.getPacient() && r.programare.getPacient()->getAsigurare())
        out << " (90% reducere - asigurat)";
    out << endl << "------------------------" << endl;
    return out;
}

istream& operator>>(istream& in, Reteta& r) {
    int idProg;
    Programare* prog = nullptr;
    do {
        cout << "Id-ul programarii: ";
        in >> idProg;
        const list<Programare*>& all = Programare::getProgramari();
        auto it = find_if(all.begin(), all.end(),
                          [idProg](Programare* p){ return p->getIdProgramare() == idProg; });
        if (it != all.end()) prog = *it;
        if (!prog)
            cout << "Programare inexistenta!" << endl;
    } while (!prog);
    r.programare = *prog;

    int n; cout << "Numar medicamente (max 10): ";
    in >> n;
    if (n < 0) n = 0;
    if (n > 10) {
        cout << "Limitat la 10." << endl;
        n = 10;
    }
    r.nrMedicamente = n;
    for (int i = 0; i < n; i++) {
        cout << "Medicament " << (i + 1) << ": ";
        in >> r.medicamente[i];
        cout << "Pret: ";
        in >> r.pretMedicamente[i];
    }
    r.recalculeazaTotal();
    return in;
}

class ClinicaManager {
private:
    static vector<Persoana*> persoane;
public:
    static void adauga(Persoana* p) {
        persoane.push_back(p);
    }

    //afiseaza polimorfic toate persoanele
    static void afiseazaToate() {
        if (persoane.empty()) {
            cout << "Nicio persoana inregistrata!" << endl;
            return;
        }
        cout << "----- Toate persoanele inregistrate in sistem ------" << endl;
        for (size_t i = 0; i < persoane.size(); i++) {
            cout << "[" << i << "] Tip real: " << persoane[i]->getRol() << endl;
            persoane[i]->afiseaza();
            cout << endl;
        }
        cout << "-------------------------------------------------" << endl;
    }
    // Folosim dynamic_cast in loc de static_cast deoarece colectia 'persoane'
    // contine pointeri polimorfici la clasa de baza (Persoana).
    // dynamic_cast verifica la runtime tipul real si returneaza nullptr daca obiectul
    // nu este un MedicChirurg, prevenind un crash, in timp ce static_cast ar forta
    // o conversie nesigura si ar cauza Undefined Behavior.

    static void afiseazaDoarChirurgi() {
        cout << "--- Chirurgi ---" << endl;
        int gasiti = 0;
        for (auto p : persoane) {
            if (MedicChirurg* mc = dynamic_cast<MedicChirurg*>(p)) {
                mc->afiseaza();
                gasiti++;
            }
        }
        if (gasiti == 0) cout << "Niciun chirurg gasit." << endl;
    }

    // s-a folosit dynamic_cast fin aceleasi motive ca mai sus
    static void afiseazaDoarPacientiCronici() {
        cout << "--- Pacienti cronici ---" << endl;
        int gasiti = 0;
        for (auto p : persoane) {
            if (PacientCronic* pc = dynamic_cast<PacientCronic*>(p)) {
                pc->afiseaza();
                gasiti++;
            }
        }
        if (gasiti == 0) cout << "Niciun pacient cronic." << endl;
    }

    static void clearArrayDoarPointeri() {
        // Nu dam delete la elemente aici daca ele sunt deja sterse din vectorii de Medici/Pacienti
        persoane.clear();
    }

    static void demoDistrugerePolimorfica() {
        cout << "--- Demo distrugere polimorfica ---" << endl;
        Persoana* p1 = new Medic("DemoMedic", 40, 'C', 5000.0f, true);
        Persoana* p2 = new MedicChirurg("DemoChirurg", 45, 'C', 8000.0f, true,50, "Cardiologie");

        vector<Medic*>& mv = const_cast<vector<Medic*>&>(Medic::getMedici());
        mv.erase(remove(mv.begin(), mv.end(), static_cast<Medic*>(p1)), mv.end());
        mv.erase(remove(mv.begin(), mv.end(), static_cast<Medic*>(p2)), mv.end());

        cout << "Se sterge p1 (Medic) prin Persoana*" << endl; delete p1;
        cout << "Se sterge p2 (MedicChirurg) prin Persoana*" << endl; delete p2;
    }

    static int getNrPersoane() { return (int)persoane.size(); }
    static const vector<Persoana*>& getPersoane() { return persoane; }
};
vector<Persoana*> ClinicaManager::persoane;


// Produs 1 abstract
class CalculatorPret {
public:
    virtual ~CalculatorPret() = default;
    // Calculeaza pretul final pornind de la subtotal
    virtual float calculeazaTotal(float subtotal) const = 0;
    virtual string getDescriere() const = 0;
};

// Produs abstract 2
class DocumentMedical {
public:
    virtual ~DocumentMedical() = default;
    virtual string genereaza(const string& numePacient, float pretTotal) const = 0;
};

// Familia public (pacientii asigurati cu reducere 90%)
class CalculatorPretPublic : public CalculatorPret {
public:
    float calculeazaTotal(float subtotal) const override {
        return subtotal * 0.10f;  // pacient asigurat plateste 10%
    }
    string getDescriere() const override {
        return "Regim public (asigurat) - reducere 90%";
    }
};

class DocumentMedicalPublic : public DocumentMedical {
public:
    string genereaza(const string& numePacient, float pretTotal) const override {
        ostringstream oss;
        oss << "--- Raport (Sistem Public) ---\n"
            << "Pacient asigurat: " << numePacient << "\n"
            << "Suma de plata: " << pretTotal << " RON (10% din total)\n"
            << "Aceasta este o factura emisa de Casa Nationala de Asigurari.";
        return oss.str();
    }
};

// Familia privat (pacientii fara asigurare care platesc integrala)
class CalculatorPretPrivat : public CalculatorPret {
public:
    float calculeazaTotal(float subtotal) const override {
        return subtotal;
    }
    string getDescriere() const override {
        return "Regim privat - plata integrala";
    }
};

class DocumentMedicalPrivat : public DocumentMedical {
public:
    string genereaza(const string& numePacient, float pretTotal) const override {
        ostringstream oss;
        oss << "--- Raport (Sistem Privat) --- \n"
            << "Pacient: " << numePacient << "\n"
            << "Suma de plata: " << pretTotal << " RON\n"
            << "Plata se face la receptia clinicii.";
        return oss.str();
    }
};

// Fabrica abstracta
class FabricaRegim {
public:
    virtual ~FabricaRegim() = default;
    virtual CalculatorPret* creeazaCalculator() const = 0;
    virtual DocumentMedical* creeazaDocument() const = 0;
    virtual string getNumeRegim() const = 0;
};

// Fabrica concreta 1 pentru regim public
class FabricaPublica : public FabricaRegim {
public:
    CalculatorPret* creeazaCalculator() const override {
        return new CalculatorPretPublic();
    }
    DocumentMedical* creeazaDocument() const override {
        return new DocumentMedicalPublic();
    }
    string getNumeRegim() const override { return "PUBLIC"; }
};

// Fabrica concreta 2 pentru privat
class FabricaPrivata : public FabricaRegim {
public:
    CalculatorPret* creeazaCalculator() const override {
        return new CalculatorPretPrivat();
    }
    DocumentMedical* creeazaDocument() const override {
        return new DocumentMedicalPrivat();
    }
    string getNumeRegim() const override { return "PRIVAT"; }
};

// "client"
class ServiciuFacturare {
private:
    FabricaRegim* fabrica;
public:
    explicit ServiciuFacturare(FabricaRegim* fab) : fabrica(fab) {
        if (fab == nullptr)
            throw ClinicaException("Fabrica nullptr");
    }
    
    void proceseazaReteta(float subtotal, const string& numePacient) {
        CalculatorPret* calc = fabrica->creeazaCalculator();
        DocumentMedical* doc = fabrica->creeazaDocument();
        
        try {
            cout << "Regim aplicat: " << fabrica->getNumeRegim() << endl;
            cout << calc->getDescriere() << endl;
            float total = calc->calculeazaTotal(subtotal);
            cout << "Subtotal: " << subtotal << " RON" << endl;
            cout << "Total calculat: " << total << " RON" << endl;
            cout << doc->genereaza(numePacient, total) << endl;
        }
        catch (...) {
            delete calc;
            delete doc;
            throw;
        }
        delete calc;
        delete doc;
    }
    string getNumeRegim() const {
        return fabrica->getNumeRegim();
    }
};


class MeniuPacient {
private:
    static int optiune;
public:
    static void showMenu() {
        do {
            cout << endl << "------- Clinica - PACIENT -------" << endl;
            cout << "1. Inregistreaza-te ca pacient nou" << endl;
            cout << "2. Inregistreaza-te ca pacient cronic " << endl;
            cout << "3. Afiseaza toti medicii" << endl;
            cout << "4. Afiseaza toti pacientii" << endl;
            cout << "5. Modifica datele tale" << endl;
            cout << "6. Sterge contul tau" << endl;
            cout << "7. Programeaza o consultatie" << endl;
            cout << "8. Afiseaza programarile mele" << endl;
            cout << "9. Modifica o programare" << endl;
            cout << "10. Sterge o programare" << endl;
            cout << "11. Afiseaza retetele mele" << endl;
            cout << "12. Afiseaza valori medicale" << endl;
            cout << "0. Inapoi" << endl;
            cout << "Optiune: ";
            cin >> optiune;
            try {
                switch (optiune) {
                    case 1: ClinicaManager::adauga(Pacient::adaugaPacientMeniu()); break;
                    case 2: adaugaPacientCronic(); break;
                    case 3: Medic::afiseazaTotiMedicii(); break;
                    case 4: Pacient::afiseazaPacienti(); break;
                    case 5: Pacient::modificaPacient(); break;
                    case 6: Pacient::stergePacient(); break;
                    case 7: Programare::createProgramare(); break;
                    case 8: Programare::afiseazaProgramariPacient(); break;
                    case 9: Programare::modificaProgramare(); break;
                    case 10: Programare::stergeProgramare(); break;
                    case 11: Reteta::afiseazaRetetePacient(); break;
                    case 12: Pacient::afiseazaValoriMedicale(); break;
                    case 0: break;
                    default: cout << "Optiune invalida!" << endl;
                }
            }
            catch (const ValidareException& e) {
                cout << "Eroare de validare: " << e.what() << endl;
                cout << "  Camp invalid: " << e.getCamp() << endl;
            }
            catch (const EntitateInexistentaException& e) {
                cout << "Nu am gasit: " << e.what() << endl;
            }
            catch (const AccesRefuzatException& e) {
                cout << "Acces refuzat: " << e.what() << endl;
            }
            catch (const ClinicaException& e) {
                cout << "Eroare clinica: " << e.what() << endl;
            }
            catch (const exception& e) {
                cout << "Eroare neasteptata: " << e.what() << endl;
            }
            catch (...) {
                cout << "Eroare necunoscuta!" << endl;
            }
        } while (optiune != 0);
    }

    static void adaugaPacientCronic() {
        char cnp[32], nume[256], afectiune[256];
        int varsta, grad, asig, nrVal;
        cout << "Nume: "; cin >> ws; cin.getline(nume, 256);
        do { cout << "CNP (13 cifre): "; cin >> cnp; }
        while (!Pacient::esteValidCnp(cnp) || Pacient::cnpExistent(cnp));

        cout << "Varsta: "; cin >> varsta;
        cout << "Asigurare(1/0): "; cin >> asig;
        cout << "Numar valori medicale (>=0): "; cin >> nrVal;

        if (nrVal < 0) nrVal = 0;
        vector<float> valori;
        if (nrVal > 0) {
            for (int i = 0; i < nrVal; i++) {
                float val;
                cout << "Valoarea " << (i + 1) << ": ";
                cin >> val;
                valori.push_back(val);
            }
        }
        cout << "Afectiune cronica: "; cin >> ws; cin.getline(afectiune, 256);
        cout << "Grad gravitate (1-5): "; cin >> grad;
        if (grad < 1) grad = 1;
        if (grad > 5) grad = 5;

        PacientCronic* pc = new PacientCronic(cnp, nume, varsta, asig != 0, valori, afectiune, grad);
        ClinicaManager::adauga(pc);
    }
};
int MeniuPacient::optiune = -1;


class MeniuMedic {
private:
    static int optiune;
public:
    static void showMenu() {
        do {
            cout << endl;
            cout << "------- Clinica - MEDIC -------" << endl;
            cout << "1. Adauga medic nou" << endl;
            cout << "2. Adauga medic chirurg nou" << endl;
            cout << "3. Afiseaza toti medicii" << endl;
            cout << "4. Cauta un medic" << endl;
            cout << "5. Afiseaza salariu medic" << endl;
            cout << "6. Modifica salariul" << endl;
            cout << "7. Sterge un medic" << endl;
            cout << "8. Afiseaza programarile mele (dupa id)" << endl;
            cout << "9. Prescrie o reteta" << endl;
            cout << "10. Modifica o reteta" << endl;
            cout << "11. Sterge o reteta" << endl;
            cout << "0. Inapoi" << endl;
            cout << "Optiune: "; cin >> optiune;
            try {
                switch (optiune) {
                    case 1: ClinicaManager::adauga(Medic::adaugaMedicMeniu()); break;
                    case 2: adaugaMedicChirurg(); break;
                    case 3: Medic::afiseazaTotiMedicii(); break;
                    case 4: Medic::cautaMedic(); break;
                    case 5: Medic::afiseazaSalariu(); break;
                    case 6: Medic::modificaSalariu(); break;
                    case 7: Medic::stergeMedicDupaId(); break;
                    case 8: Programare::afiseazaProgramariMedic(); break;
                    case 9: Reteta::adaugaRetetaMeniu(); break;
                    case 10: Reteta::modificaReteta(); break;
                    case 11: Reteta::stergeReteta(); break;
                    case 0: break;
                    default: cout << "Optiune invalida!" << endl;
                }
            }
            catch (const ValidareException& e) {
                cout << "Eroare de validare: " << e.what() << endl;
            }
            catch (const EntitateInexistentaException& e) {
                cout << "Nu am gasit: " << e.what() << endl;
            }
            catch (const AccesRefuzatException& e) {
                cout << "Acces refuzat: " << e.what() << endl;
            }
            catch (const ClinicaException& e) {
                cout << "Eroare clinica: " << e.what() << endl;
            }
            catch (const exception& e) {
                cout << "Eroare neasteptata: " << e.what() << endl;
            }
            catch (...) {
                cout << "Eroare necunoscuta!" << endl;
            }
        } while (optiune != 0);
    }

    static void adaugaMedicChirurg() {
        char nume[256], sectie[256];
        char specBuf[16];
        char spec;
        int varsta, op, activ;
        float salariu;
        cout << "Numele medicului: ";
        cin >> ws; cin.getline(nume, 256);
        cout << "Varsta: "; cin >> varsta;
        do {
            cout << "Specializarea (1 caracter, ex: C/N/P): ";
            cin >> ws; cin.getline(specBuf, 16);
        } while (string(specBuf).length() != 1);
        spec = specBuf[0];
        cout << "Salariul: "; cin >> salariu;
        cout << "Activ (1=Da, 0=Nu): "; cin >> activ;
        cout << "Sectie: "; cin >> ws; cin.getline(sectie, 256);
        cout << "Numar operatii: "; cin >> op;
        MedicChirurg* mc = new MedicChirurg(nume, varsta, spec, salariu, activ != 0, op, sectie);
        ClinicaManager::adauga(mc);
        cout << "Chirurg adaugat (id=" << mc->getIdMedic() << ")" << endl;
    }
};
int MeniuMedic::optiune = -1;


class MeniuRapoarte {
private:
    static int optiune;
public:

    static void filtreazaMediciDupaSpecializare() {
        if (Medic::getMedici().empty()) {
            cout << "Nu exista medici!" << endl;
            return;
        }
        char spec;
        cout << "Specializare (C/N/P/O): ";
        cin >> spec;
        vector<Medic*> rezultat;
        copy_if(Medic::getMedici().begin(), Medic::getMedici().end(),
                back_inserter(rezultat),
                [spec](Medic* m) {
                    return m->getSpecializare() == spec;
                });
        if (rezultat.empty()) {
            cout << "Niciun medic cu specializarea " << spec << endl;
            return;
        }
        cout << "Medici cu specializarea " << spec << ":" << endl;
        for_each(rezultat.begin(), rezultat.end(), [](Medic* m) {
            cout << *m << endl;
        });
    }

    static void sorteazaMediciDupaSalariu() {
        if (Medic::getMedici().empty()) {
            cout << "Nu exista medici!" << endl;
            return;
        }
        vector<Medic*> copie(Medic::getMedici().begin(), Medic::getMedici().end());
        sort(copie.begin(), copie.end(),
             [](Medic* a, Medic* b){ return a->getSalariu() > b->getSalariu(); });
        cout << "Medici sortati dupa salariu (descrescator):" << endl;
        for_each(copie.begin(), copie.end(), [](Medic* m){
            cout << "  - " << m->getNume() << ": " << m->getSalariu() << " RON" << endl;
        });
    }

    static void numaraPacientiAsigurati() {
        int n = (int)count_if(Pacient::getPacienti().begin(), Pacient::getPacienti().end(),
                              [](Pacient* p){ return p->getAsigurare(); });
        cout << "Pacienti asigurati: " << n << " din " << Pacient::getNrPacienti() << endl;
    }

    static void showMenu() {
        do {
            cout << endl;
            cout << " ----- CENTRUL DE RAPORTARE -------  " << endl;
            cout << "1. Afiseaza toate persoanele " << endl;
            cout << "2. Afiseaza doar chirurgii " << endl;
            cout << "3. Afiseaza doar pacientii cronici " << endl;
            cout << "4. Filtreaza medici dupa specializare" << endl;
            cout << "5. Sorteaza medici dupa salariu" << endl;
            cout << "6. Numara pacienti asigurati" << endl;
            cout << "7. Afiseaza jurnalul de evenimente" << endl;
            cout << "0. Inapoi" << endl;
            cout << "Optiune: "; cin >> optiune;
            try {
                switch (optiune) {
                    case 1: ClinicaManager::afiseazaToate(); break;
                    case 2: ClinicaManager::afiseazaDoarChirurgi(); break;
                    case 3: ClinicaManager::afiseazaDoarPacientiCronici(); break;
                    case 4: filtreazaMediciDupaSpecializare(); break;
                    case 5: sorteazaMediciDupaSalariu(); break;
                    case 6: numaraPacientiAsigurati(); break;
                    case 7: Jurnal::getInstance().afiseaza(); break;
                    case 0: break;
                    default: cout << "Optiune invalida!" << endl;
                }
            }
            catch (const exception& e) {
                cout << "Eroare: " << e.what() << endl;
            }
            catch (...) {
                cout << "Eroare necunoscuta!" << endl;
            }
        } while (optiune != 0);
    }
};
int MeniuRapoarte::optiune = -1;


class MeniuTarifare {
private:
    static int optiune;
    static FabricaRegim* fabricaCurenta;
public:
    static FabricaRegim* getFabrica() {
        if (fabricaCurenta == nullptr) {
            fabricaCurenta = new FabricaPublica();
        }
        return fabricaCurenta;
    }

    static void veziRegimCurent() {
        cout << "Regim curent: " << getFabrica()->getNumeRegim() << endl;
    }

    static void schimbaRegim() {
        cout << "Regim curent: " << getFabrica()->getNumeRegim() << endl;
        cout << "1. Public (asigurat - reducere 90%)" << endl;
        cout << "2. Privat (plata integrala)" << endl;
        cout << "Alege: ";
        int opt; cin >> opt;
        if (fabricaCurenta) delete fabricaCurenta;
        if (opt == 1)      fabricaCurenta = new FabricaPublica();
        else if (opt == 2) fabricaCurenta = new FabricaPrivata();
        else {
            fabricaCurenta = new FabricaPublica();
            cout << "Optiune invalida, s-a setat default: Public" << endl;
        }
        cout << "Regim setat: " << fabricaCurenta->getNumeRegim() << endl;
    }

    static void ProcesareReteta() {
        if (Reteta::getRetete().empty()) {
            cout << "Nu exista retete pentru procesare!" << endl;
            return;
        }
        long long id;
        cout << "Id reteta: ";
        cin >> id;
        auto it = Reteta::getRetete().find(id);
        if (it == Reteta::getRetete().end()) {
            cout << "Reteta inexistenta!" << endl;
            return;
        }
        Reteta* r = it->second;
        float subtotal = 0;
        for (int i = 0; i < r->getNrMedicamente(); i++)
            subtotal += r->getPretMedicament(i);
        ServiciuFacturare proc(getFabrica());
        string numePac = r->getProgramare().getPacient()
                         ? r->getProgramare().getPacient()->getNume() : "Necunoscut";
        proc.proceseazaReteta(subtotal, numePac);
    }

    static void showMenu() {
        do {
            cout << endl;
            cout << "------ REGIM DE TARIFARE ------" << endl;
            cout << "1. Vezi regimul curent" << endl;
            cout << "2. Schimba regimul (Public sau Privat)" << endl;
            cout << "3. Proceseaza o reteta cu regimul curent" << endl;
            cout << "0. Inapoi" << endl;
            cout << "Optiune: "; cin >> optiune;
            try {
                switch (optiune) {
                    case 1: veziRegimCurent(); break;
                    case 2: schimbaRegim(); break;
                    case 3: ProcesareReteta(); break;
                    case 0: break;
                    default: cout << "Optiune invalida!" << endl;
                }
            }
            catch (const exception& e) {
                cout << "Eroare: " << e.what() << endl;
            }
            catch (...) {
                cout << "Eroare necunoscuta!" << endl;
            }
        } while (optiune != 0);
    }
    static void elibereaza() {
        if (fabricaCurenta) { delete fabricaCurenta; fabricaCurenta = nullptr; }
    }
};
int MeniuTarifare::optiune = -1;
FabricaRegim* MeniuTarifare::fabricaCurenta = nullptr;


class MeniuTeste {
private:
    static int optiune;
public:
    static void demoStackUnwinding() {
        cout << "\n----- Demo Stack Unwinding -----" << endl;
        try {
            cout << "1. Se creaza un Medic temporar pe stiva" << endl;
            Medic temp("DR. DEMO STACK", 'C');
            vector<Medic*>& mv = const_cast<vector<Medic*>&>(Medic::getMedici());
            mv.erase(remove(mv.begin(), mv.end(), &temp), mv.end());
            cout << "2. Se arunca o ValidareException" << endl;
            throw ValidareException("camp_demo", "valoare_demo", "exceptie de test");
            cout << "3. Nu se ajunge aici." << endl;
        }
        catch (const ValidareException& e) {
            cout << "Exceptie prinsa: " << e.what() << endl;
            cout << "Destructorul lui Medic a rulat AUTOMAT in timpul stack unwinding-ului." << endl;
        }
    }

    static void showMenu() {
        do {
            cout << endl;
            cout << " ------ VERIFICARE CERINTE  ------ " << endl;
            cout << "1. Demo Stack Unwinding (exceptii)" << endl;
            cout << "2. Demo Distrugere Polimorfica (delete prin pointer la baza)" << endl;
            cout << "0. Inapoi" << endl;
            cout << "Optiune: "; cin >> optiune;
            try {
                switch (optiune) {
                    case 1: demoStackUnwinding(); break;
                    case 2: ClinicaManager::demoDistrugerePolimorfica(); break;
                    case 0: break;
                    default: cout << "Optiune invalida!" << endl;
                }
            }
            catch (const exception& e) {
                cout << "Eroare neasteptata in demo: " << e.what() << endl;
            }
            catch (...) {
                cout << "Eroare necunoscuta!" << endl;
            }
        } while (optiune != 0);
    }
};
int MeniuTeste::optiune = -1;


class MeniuPrincipal {
private:
    static int optiune;
public:
    static void showMenu() {
        do {
            cout << endl;
            cout << " ------ CLINICA GENERAL ------ " << endl;
            cout << "1. Acces ca Pacient" << endl;
            cout << "2. Acces ca Medic " << endl;
            cout << "3. Centrul de Raportare " << endl;
            cout << "4. Regim de Tarifare   " << endl;
            cout << "5. Verificare Cerinte   " << endl;
            cout << "0. Iesire" << endl;
            cout << "Optiune: "; cin >> optiune;
            try {
                switch (optiune) {
                    case 1: MeniuPacient::showMenu();  optiune = -1; break;
                    case 2: MeniuMedic::showMenu();    optiune = -1; break;
                    case 3: MeniuRapoarte::showMenu(); optiune = -1; break;
                    case 4: MeniuTarifare::showMenu(); optiune = -1; break;
                    case 5: MeniuTeste::showMenu();    optiune = -1; break;
                    case 0: cout << "La revedere!" << endl; break;
                    default: cout << "Optiune invalida!" << endl;
                }
            }
            catch (const exception& e) {
                cout << "Eroare la nivel principal: " << e.what() << endl;
            }
            catch (...) {
                cout << "Eroare necunoscuta!" << endl;
            }
        } while (optiune != 0);
    }
};
int MeniuPrincipal::optiune = -1;


int main() {
    Jurnal::getInstance().log("Aplicatia a pornit");

    // Următoarele linii ar genera erori de compilare dacă ar fi decomentate,
    // demonstrând că nu se pot crea mai multe instanțe ale clasei Jurnal:

    // Jurnal j1;                                 // Eroare: constructorul implicit este privat
    // Jurnal j2 = Jurnal::getInstance();         // Eroare: constructorul de copiere este sters (= delete)
    // Jurnal j3; j3 = Jurnal::getInstance();     // Eroare: operatorul de atribuire este sters (= delete)

    MeniuPrincipal::showMenu();


    Reteta::stergeRetete();
    Programare::stergeProgramari();
    ClinicaManager::clearArrayDoarPointeri();
    Medic::stergeMedici();
    Pacient::stergePacienti();
    MeniuTarifare::elibereaza();
    return 0;
}

