#include "../Comportamientos_Jugador/rescatador.hpp"
#include "motorlib/util.h"
#include <algorithm>
#include <climits>
#include <iostream>
#include <numeric>
#include <queue>

double rand01(const Sensores& s) {
    return (double)std::rand() / (RAND_MAX + 1.0);
}

Action ComportamientoRescatador::think(Sensores sensores)
{

	switch (sensores.nivel)
	{
	case 0:
		return ComportamientoRescatadorNivel_0(sensores);
	case 1:
		return ComportamientoRescatadorNivel_1 (sensores);
		break;
	case 2:
		return ComportamientoRescatadorNivel_2 (sensores);
		break;
	case 3:
		return ComportamientoRescatadorNivel_3 (sensores);
		break;
	case 4:
		// accion = ComportamientoRescatadorNivel_4 (sensores);
		break;
	default:
		return IDLE;
	}

	return IDLE;
}

int ComportamientoRescatador::interact(Action, int) { return 0; }

/* ----------  NIVEL 0  ---------- */
Action ComportamientoRescatador::ComportamientoRescatadorNivel_0(Sensores s)
{
  actualizarEstado(s);
  return tomarDecisionNivel_0(s);
}

void ComportamientoRescatador::actualizarEstado(const Sensores &sensores) {
    // 1. Actualizar equipamiento (bikini/zapatillas)
    if (sensores.superficie[0] == 'K') {
        tieneBikini = true;
    }
    if (sensores.superficie[0] == 'D') {
        tieneZapatillas = true;
    }

    // 3. Actualizar mapa resultado con lo que ve
    rellenarMapa(sensores, mapaResultado, true);

    // 4. Actualizar mapa de cotas (opcional, según necesidades)
    rellenarMapa(sensores, mapaCotas, false);

    // 5. Para nivel 2: Verificar si hemos sido interrumpidos
    if (!plan.empty()) {
        // Comprobar si nuestra posición actual coincide con la esperada
        // Si no, limpiar el plan porque hemos sido desviados
        if (!estaEnCaminoEsperado(sensores)) {
            plan.clear();
        }
    }
}

bool ComportamientoRescatador::estaEnCaminoEsperado(const Sensores &sensores) const {
    // Implementación básica - puede ser más sofisticada
    // Por ahora simplemente verifica si estamos en el camino esperado
    // En una implementación real, llevaría registro de la posición esperada
    
    return true; // Versión simplificada
}

bool ComportamientoRescatador::casillaAccesible(const Sensores &s, int idx) const {
    if (s.agentes[idx] != '_') {
        return false; // Casilla ocupada
    }
    
    int dh = abs(s.cota[0] - s.cota[idx]); // Diferença de altura
    char terreno = s.superficie[idx];
    
    // Verifica desnível máximo permitido
    bool alturaOK = false;
    if (tieneZapatillas) {
        alturaOK = (dh <= 2); // Com zapatillas pode subir até 2 níveis
    } else {
        alturaOK = (dh <= 1); // Sem zapatillas só 1 nível
    }
    
    if (!alturaOK) {
        return false;
    }
    
    switch (terreno) {
        case 'P':
        case 'M':
            return false;
        case 'B':
            return tieneZapatillas;
        case 'A':
            return tieneBikini;
        default:
            return true;
    }
}

Action ComportamientoRescatador::tomarDecisionNivel_1(const Sensores& s) {
    // 0) Se já está na base, parar
    if (s.superficie[0] == 'X') return IDLE;


    if (leftCounter > 0) {
        leftCounter--;
        return TURN_SR;
    }

    struct Opcion {
        Action accion;
        int prioridad;
        char tipo;
    };

    vector<Opcion> opciones;
    
    auto evaluarCasilla = [&](int idx, Action accion) {
        if (!casillaAccesible(s, idx)) return;
        
        char t = s.superficie[idx];
        int peso = 0;
        
        if (t == '?') peso = 1000;  
        else if ((t == 'C' || t == 'S') && !yaDescubierto(s, idx)) {
            peso = (t == 'C') ? 500 : 400;
        }
        else if (!tieneZapatillas && t == 'D') peso = 300;
        else if (!tieneBikini && t == 'K') peso = 250;
        else if (t == 'C' || t == 'S') peso = 100;
        
        if (peso > 0) {
            opciones.push_back({accion, peso, t});
        }
    };

    evaluarCasilla(1, TURN_L); 
    evaluarCasilla(2, WALK);
    evaluarCasilla(3, TURN_SR);

    if (!opciones.empty()) {
        int total = accumulate(opciones.begin(), opciones.end(), 0, 
            [](int sum, const Opcion& op) { return sum + op.prioridad; });
        
        int randVal = rand() % total;
        int acum = 0;
        
        for (auto& op : opciones) {
            acum += op.prioridad;
            if (randVal < acum) {
                cout << "Escolha: " << op.accion << " para " << op.tipo 
                     << " (peso: " << op.prioridad << "/" << total << ")" << endl;

                if (op.accion == TURN_L) leftCounter = 1;
                return op.accion;
            }
        }
    }

    leftCounter = (rand() % 3);
    return TURN_SR;
}


inline bool ComportamientoRescatador::caminoViable(const Sensores &s, int idx) const
{
  if (s.agentes[idx] != '_')
    return false;
  int dh = abs(s.cota[0] - s.cota[idx]);
  if (dh >= 2)
    return false;
  char t = s.superficie[idx];
  return t == 'C' || t == 'X';
}

Action ComportamientoRescatador::tomarDecisionNivel_0(const Sensores& s)
{
    if (s.superficie[0]=='X') return IDLE;

    if (leftCounter>0){
        --leftCounter;
        ultimaAccion = TURN_SR;
        return TURN_SR;
    }


    int best=-1, bestDist=INT_MAX;
    for(int j=1;j<16;++j) if (s.superficie[j]=='X'){
        DFC dX=calcularDiferencias(j,s.rumbo);
        for(int i:{1,2,3}) if (caminoViable(s,i)){
            DFC dI=calcularDiferencias(i,s.rumbo);
            int m=abs(dX.df-dI.df)+abs(dX.dc-dI.dc);
            if(m<bestDist){bestDist=m;best=i;}
        }
    }
    if(best!=-1){
        if(best==2)           return WALK;
        if(best==3)           return TURN_SR;
        leftCounter=6;        return TURN_SR; 
    }

    int wF = caminoViable(s,2) ? 3 : 0;
    int wL = caminoViable(s,1) ? 2 : 0;
    int wR = caminoViable(s,3) ? 1 : 0;
    int total = wF + wL + wR;


    if (total>0)
    {
        double r = rand01(s) * total;
        if (r < wF) { ultimaAccion=WALK;           return WALK; }
        r -= wF;
        if (r < wL) { leftCounter=6; ultimaAccion=TURN_SR; return TURN_SR; }
        ultimaAccion=TURN_SR;
        return TURN_SR;
    }

    ultimaAccion = TURN_SR;
    return TURN_SR;
}

Action ComportamientoRescatador::ComportamientoRescatadorNivel_1(Sensores sensores)
{
	actualizarEstado(sensores);
    return tomarDecisionNivel_1(sensores);
}


bool ComportamientoRescatador::yaDescubierto(const Sensores& s, int idx) {
	int fil = s.posF;
	int col = s.posC;
	
	if (idx == 1) { fil--; col++; }
	else if (idx == 2) { fil++; }
	else if (idx == 3) { fil--; col--; }
	else if (idx >= 4 && idx <= 8) {
		fil += (idx % 2) ? -1 : 1;
		col += (idx < 6) ? 1 : -1;
	}
	
	return celdasCyS.count({fil, col}) > 0;
}



Action ComportamientoRescatador::ComportamientoRescatadorNivel_2(Sensores sensores) {
    actualizarEstado(sensores);
    
    if (sensores.posF == sensores.destinoF && sensores.posC == sensores.destinoC) {
        return IDLE;
    }
    
    if (!plan.empty()) {
        Action siguiente = plan.front();
        plan.pop_front();
        return siguiente;
    }
    
    calcularCaminoDijkstra(sensores);
    
    if (!plan.empty()) {
        Action siguiente = plan.front();
        plan.pop_front();
        return siguiente;
    }
    
    return buscarAlternativa(sensores);
}

void ComportamientoRescatador::calcularCaminoDijkstra(const Sensores &sensores) {
    plan.clear();
    
    const int filas = mapaResultado.size();
    const int columnas = filas > 0 ? mapaResultado[0].size() : 0;
    if (filas == 0 || columnas == 0) return;

    vector<vector<int>> dist(filas, vector<int>(columnas, INT_MAX));
    vector<vector<pair<int, int>>> prev(filas, vector<pair<int, int>>(columnas, {-1, -1}));
    vector<vector<bool>> visitado(filas, vector<bool>(columnas, false));

    priority_queue<tuple<int, int, int>, vector<tuple<int, int, int>>, greater<tuple<int, int, int>>> pq;

    int inicioF = sensores.posF;
    int inicioC = sensores.posC;
    int destinoF = sensores.destinoF;
    int destinoC = sensores.destinoC;

    dist[inicioF][inicioC] = 0;
    pq.push({0, inicioF, inicioC});

    const int dirs[8][2] = {{-1,0}, {-1,1}, {0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}};

    while (!pq.empty()) {
        auto [d, f, c] = pq.top();
        pq.pop();
        
        if (visitado[f][c]) continue;
        visitado[f][c] = true;
        
        if (f == destinoF && c == destinoC) break;

        for (int i = 0; i < 8; ++i) {
            int nf = f + dirs[i][0];
            int nc = c + dirs[i][1];
         
            if (!esCasillaAccesible(nf, nc, sensores)) continue;
            
            int costo = (dirs[i][0] != 0 && dirs[i][1] != 0) ? 14 : 10;
            costo *= getCostoTerreno(mapaResultado[nf][nc]);
            
            if (d + costo < dist[nf][nc]) {
                dist[nf][nc] = d + costo;
                prev[nf][nc] = {f, c};
                pq.push({dist[nf][nc], nf, nc});
            }
        }
    }

    if (dist[destinoF][destinoC] != INT_MAX) {
        vector<pair<int, int>> camino;
        pair<int, int> actual = {destinoF, destinoC};
        
        while (actual.first != inicioF || actual.second != inicioC) {
            camino.push_back(actual);
            actual = prev[actual.first][actual.second];
        }
        
        reverse(camino.begin(), camino.end());
        convertirCaminoAAcciones(camino, sensores);
    }
}

void ComportamientoRescatador::convertirCaminoAAcciones(const vector<pair<int, int>>& camino, const Sensores &sensores) {
    int rumbo = sensores.rumbo;
    int posF = sensores.posF;
    int posC = sensores.posC;
    
    for (const auto& [f, c] : camino) {
        int df = f - posF;
        int dc = c - posC;

        int dirDeseada;
        if (df == -1 && dc == 0) dirDeseada = 0; 
        else if (df == -1 && dc == 1) dirDeseada = 1; 
        else if (df == 0 && dc == 1) dirDeseada = 2; 
        else if (df == 1 && dc == 1) dirDeseada = 3; 
        else if (df == 1 && dc == 0) dirDeseada = 4; 
        else if (df == 1 && dc == -1) dirDeseada = 5; 
        else if (df == 0 && dc == -1) dirDeseada = 6; 
        else if (df == -1 && dc == -1) dirDeseada = 7; 
        
        int dif = (dirDeseada - rumbo + 8) % 8;
        
        if (dif > 0) {
            if (dif <= 4) {
                for (int i = 0; i < dif; ++i) {
                    plan.push_back(TURN_SR);
                    rumbo = (rumbo + 1) % 8;
                }
            } else {
                for (int i = 0; i < 8 - dif; ++i) {
                    plan.push_back(TURN_L);
                    rumbo = (rumbo + 7) % 8;
                }
            }
        }
        
        plan.push_back(WALK);
        posF = f;
        posC = c;
    }
}

bool ComportamientoRescatador::esTerrenoTransitable(char terreno) const {
    switch (terreno) {
        case 'P':
        case 'M':
            return false;

        case 'B':
            return tieneZapatillas;

        case 'A':
            return true;

        case 'T':
        case 'S':
        case 'C':
        case 'X':
        case 'D':
        case 'K':
        case 'G':
            return true;

        case '?': 
            return true;


        default:
            return false;
    }
}

bool ComportamientoRescatador::esTerrenoTransitableCompleto(char terreno, int desnivel) const {
    if (!esTerrenoTransitable(terreno)) {
        return false;
    }

    int maxDesnivel = tieneZapatillas ? 2 : 1;
    if (abs(desnivel) > maxDesnivel) {
        return false;
    }

    return true;
}

bool ComportamientoRescatador::esCasillaAccesible(int fila, int columna, const Sensores &sensores) const {
	if (fila < 0 || fila >= mapaResultado.size() || columna < 0 || columna >= mapaResultado[0].size()) {
        return false;
    }
    
    char terreno = mapaResultado[fila][columna];
    if (!esTerrenoTransitable(terreno)) {
        return false;
    }

    int desnivel = abs(sensores.cota[0] - mapaCotas[fila][columna]);
    if (tieneZapatillas) {
        if (desnivel > 2) return false;
    } else {
        if (desnivel > 1) return false;
    }

    return true;
}

int ComportamientoRescatador::getCostoTerreno(char terreno) const {
    switch (terreno) {
        case 'A': return tieneBikini ? 1 : 3;
        case 'B': return tieneZapatillas ? 1 : 5;
        case 'T': return 2;
        default: return 1;
    }
}

Action ComportamientoRescatador::buscarAlternativa(const Sensores &sensores) {
    if (casillaAccesible(sensores, 2)) return WALK;
    if (casillaAccesible(sensores, 3)) return TURN_SR;
    if (casillaAccesible(sensores, 1)) return TURN_L;
    
    return TURN_SR;
}


Action ComportamientoRescatador::ComportamientoRescatadorNivel_3(Sensores sensores) {
    actualizarEstado(sensores);
    
    if (sensores.posF == sensores.destinoF && sensores.posC == sensores.destinoC) {
        plan.clear();
        return IDLE;
    }
    
    if (plan.empty()) {
        EstadoA inicio;
        inicio.f = sensores.posF;
        inicio.c = sensores.posC;
        inicio.brujula = sensores.rumbo;
        inicio.zapatillas = tieneZapatillas;
        inicio.bikini = tieneBikini;
        
        EstadoA objetivo;
        objetivo.f = sensores.destinoF;
        objetivo.c = sensores.destinoC;
        
        plan = aEstrella(inicio, objetivo, mapaResultado, mapaCotas);
        
        if (plan.empty()) {
            return buscarAlternativa(sensores);
        }
    }
    Action accion = plan.front();
    plan.pop_front();
    return accion;
}



list<Action> ComportamientoRescatador::aEstrella(
    const EstadoA &inicio, 
    const EstadoA &objetivo,
    const vector<vector<unsigned char>> &mapa,
    const vector<vector<unsigned char>> &alturas) {
    
    auto comparador = [](const NodoAStar& a, const NodoAStar& b) {
        return a.f > b.f;
    };
    priority_queue<NodoAStar, vector<NodoAStar>, decltype(comparador)> frontera(comparador);
    
    unordered_map<string, int> g_values;
    unordered_map<string, EstadoA> came_from;
    unordered_map<string, Action> action_from;

    auto generarClave = [](const EstadoA& e) {
        return to_string(e.f) + "," + to_string(e.c) + "," + 
               to_string(e.brujula) + "," + 
               (e.zapatillas ? "1" : "0") + "," + (e.bikini ? "1" : "0");
    };

    NodoAStar nodoInicial;
    nodoInicial.estado = inicio;
    nodoInicial.g = 0;
    nodoInicial.h = heuristica(inicio, objetivo);
    nodoInicial.f = nodoInicial.g + nodoInicial.h;
    
    frontera.push(nodoInicial);
    g_values[generarClave(inicio)] = 0;

    while (!frontera.empty()) {
        NodoAStar actual = frontera.top();
        frontera.pop();
  
        if (actual.estado.f == objetivo.f && actual.estado.c == objetivo.c) {
            return reconstruirCamino(inicio, actual.estado, came_from, action_from, generarClave);
        }

        vector<pair<Action, EstadoA>> vecinos = generarVecinosAStar(actual.estado, mapa, alturas);
        
        for (auto &[accion, vecino] : vecinos) {
            int costo_movimiento = calcularCoste(actual.estado, vecino, mapa, alturas);
            int g_temp = actual.g + costo_movimiento;
            string clave_vecino = generarClave(vecino);
  
            if (!g_values.count(clave_vecino) || g_temp < g_values[clave_vecino]) {
                g_values[clave_vecino] = g_temp;
                came_from[clave_vecino] = actual.estado;
                action_from[clave_vecino] = accion;
                
                NodoAStar nuevo_nodo;
                nuevo_nodo.estado = vecino;
                nuevo_nodo.g = g_temp;
                nuevo_nodo.h = heuristica(vecino, objetivo);
                nuevo_nodo.f = nuevo_nodo.g + nuevo_nodo.h;
                
                frontera.push(nuevo_nodo);
            }
        }
    }
    
    return {};
}

vector<pair<Action, EstadoA>> ComportamientoRescatador::generarVecinosAStar(
    const EstadoA &actual,
    const vector<vector<unsigned char>> &mapa,
    const vector<vector<unsigned char>> &alturas) {
    
    vector<pair<Action, EstadoA>> vecinos;
    
    EstadoA siguiente = siguienteCasilla(actual);
    if (casillaAccesible(actual, siguiente, mapa, alturas)) {
        vecinos.emplace_back(WALK, siguiente);
    }

    EstadoA giroD = actual;
    giroD.brujula = (Orientacion)((giroD.brujula + 1) % 8);
    vecinos.emplace_back(TURN_SR, giroD);

    EstadoA giroL = actual;
    giroL.brujula = (Orientacion)((giroL.brujula + 7) % 8);
    vecinos.emplace_back(TURN_L, giroL);
    
    return vecinos;
}

int ComportamientoRescatador::heuristica(const EstadoA &actual, const EstadoA &objetivo) {

    return (abs(actual.f - objetivo.f) + abs(actual.c - objetivo.c)) * 10;
}

bool ComportamientoRescatador::casillaAccesible(
    const EstadoA &actual,
    const EstadoA &siguiente,
    const vector<vector<unsigned char>> &mapa,
    const vector<vector<unsigned char>> &alturas) {
    
    if (siguiente.f < 0 || siguiente.f >= mapa.size() || 
        siguiente.c < 0 || siguiente.c >= mapa[0].size()) {
        return false;
    }
    
    char terreno = mapa[siguiente.f][siguiente.c];

    if (terreno == 'P' || terreno == 'M') {
        return false;
    }
    
    if (terreno == 'B' && !actual.zapatillas) {
        return false;
    }
    if (terreno == 'A' && !actual.bikini) {
        return false;
    }
    
    int dh = abs(alturas[siguiente.f][siguiente.c] - alturas[actual.f][actual.c]);
    if (dh > 1 && !actual.zapatillas) {
        return false;
    }
    if (dh > 2) {
        return false;
    }
    
    return true;
}

list<Action> ComportamientoRescatador::reconstruirCamino(
    const EstadoA &inicio,
    const EstadoA &objetivo,
    const unordered_map<string, EstadoA> &padre,
    const unordered_map<string, Action> &accion_padre,
    function<string(const EstadoA&)> generarClave) {
    
    list<Action> camino;
    EstadoA actual = objetivo;
    string clave_actual = generarClave(actual);
    
    while (padre.count(clave_actual)) {
        camino.push_front(accion_padre.at(clave_actual));
        actual = padre.at(clave_actual);
        clave_actual = generarClave(actual);
    }
    
    return camino;
}


int ComportamientoRescatador::calcularCoste(
    const EstadoA &actual, 
    const EstadoA &siguiente, 
    const vector<vector<unsigned char>> &mapa,
    const vector<vector<unsigned char>> &alturas) {
    
    char terreno = mapa[siguiente.f][siguiente.c];
    int coste = 0;

    // Coste del terreno
    switch (terreno) {
        case 'A': coste = (actual.bikini ? 10 : 100); break;
        case 'B': coste = (actual.zapatillas ? 10 : 50); break;
        case 'T': coste = 2; break;
        case 'C': coste = 10; break;
        case 'S': coste = 20; break;
        case 'X': coste = 0; break;
        default: coste = INT_MAX;
    }

    int dh = abs(alturas[siguiente.f][siguiente.c] - alturas[actual.f][actual.c]);
    coste += dh * 5;

    return coste;
}

EstadoA ComportamientoRescatador::siguienteCasilla(const EstadoA &actual) {
    EstadoA siguiente = actual;
    switch (actual.brujula) {
        case norte: siguiente.f--; break;
        case noreste: siguiente.f--; siguiente.c++; break;
        case este: siguiente.c++; break;
        case sureste: siguiente.f++; siguiente.c++; break;
        case sur: siguiente.f++; break;
        case suroeste: siguiente.f++; siguiente.c--; break;
        case oeste: siguiente.c--; break;
        case noroeste: siguiente.f--; siguiente.c--; break;
    }
    return siguiente;
}

Action ComportamientoRescatador::ComportamientoRescatadorNivel_4(Sensores sensores)
{
}