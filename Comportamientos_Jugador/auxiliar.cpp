#include "../Comportamientos_Jugador/auxiliar.hpp"
#include "rescatador.hpp"
#include <climits>
#include <iostream>
#include <list>
#include <numeric>
#include <queue>
#include <time.h>
#include <thread>


list<Action> ComportamientoAuxiliar::avanzaSaltosDeCaballo()
{
   list<Action> plan;
   plan.push_back(WALK);
   plan.push_back(WALK);
   plan.push_back(TURN_SR);
   plan.push_back(TURN_SR);
   plan.push_back(WALK);
   return plan;
}


Action ComportamientoAuxiliar::think(Sensores sensores)
{
	Action accion = IDLE;

	switch (sensores.nivel)
	{
	case 0:
		// accion = ComportamientoAuxiliarNivel_0 (sensores);
		accion = ComportamientoAuxiliarNivel_0(sensores);
		break;
	case 1:
		accion = ComportamientoAuxiliarNivel_1 (sensores);
		break;
	case 2:
		// accion = ComportamientoAuxiliarNivel_2 (sensores);
		break;
	case 3:
      accion = ComportamientoAuxiliarNivel_E(sensores);
		accion = ComportamientoAuxiliarNivel_3 (sensores);
		break;
	case 4:
		accion = ComportamientoAuxiliarNivel_4 (sensores);
		break;
	}

	return accion;
}



bool ComportamientoAuxiliar::casillaAccesible(const Sensores &s, int idx) const {
   if (s.agentes[idx] != '_') {
       return false; 
   }
   
   int dh = abs(s.cota[0] - s.cota[idx]);
   char terreno = s.superficie[idx];
   
   bool alturaOK = false;
   if (tieneZapatillas) {
       alturaOK = (dh <= 2);
   } else {
       alturaOK = (dh <= 1); 
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


Action ComportamientoAuxiliar::tomarDecisionNivel_1(const Sensores& s) {
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



EstadoA ComportamientoAuxiliar::aplicar(Action accion, const EstadoA &st, const vector<vector<unsigned char>> &terreno, const vector<vector<unsigned char>> &altura)
{
   EstadoA siguiente = st;
   cout << "estado actual: " << "fila: " << st.f << " col: " << st.c << " orientacion: " << st.brujula << endl;
   switch (accion)
   {
   case WALK:
      if (casillaAccesibleAuxiliar(st, terreno, altura))
      {
         siguiente = siguienteCasillaAuxiliar(st);
      }
      break;
   case TURN_SR:
      siguiente.brujula = cambiarOrientacion(siguiente.brujula);
      break;
   }
   cout << "estado final: " << "fila: " << siguiente.f << " col: " << siguiente.c << " orientacion: " << siguiente.brujula << endl;

   return siguiente;
}




bool ComportamientoAuxiliar::encontrar(const NodoA &st, const list<NodoA> &lista)
{
   auto it = lista.begin();
   while (it != lista.end() and !((*it) == st))
   {
      it++;
   }

   return (it != lista.end());
}


list<Action> ComportamientoAuxiliar::busquedaAnchura(const EstadoA &inicio, const EstadoA &fin, const vector<vector<unsigned char>> &terreno, const vector<vector<unsigned char>> &altura)
{
   NodoA nodoActual;
   list<NodoA> frontera;
   list<NodoA> explorados;

   nodoActual.estado = inicio;
   frontera.push_back(nodoActual);
   bool solucion = nodoActual.estado.f == fin.f and nodoActual.estado.c == fin.c;
   while (!frontera.empty() and !solucion)
   {
      frontera.pop_front();
      explorados.push_back(nodoActual);

      if (terreno[nodoActual.estado.f][nodoActual.estado.c] == 'D')
      {
         nodoActual.estado.zapatillas = true;
      }
      NodoA hijoWalk = nodoActual;
      hijoWalk.estado = aplicar(WALK, nodoActual.estado, terreno, altura);
      hijoWalk.acciones.push_back(WALK);
      solucion = hijoWalk.estado.f == fin.f and hijoWalk.estado.c == fin.c;
      if (solucion)
      {
         nodoActual = hijoWalk;
      }

      if (!encontrar(hijoWalk, frontera) and !encontrar(hijoWalk, explorados))
      {
         frontera.push_back(hijoWalk);
      }

      if (!solucion)
      {
         NodoA hijoTurnSR = nodoActual;
         hijoTurnSR.estado = aplicar(TURN_SR, nodoActual.estado, terreno, altura);
         hijoTurnSR.acciones.push_back(TURN_SR);
         if (!encontrar(hijoTurnSR, frontera) and !encontrar(hijoTurnSR, explorados))
         {
            frontera.push_back(hijoWalk);
         }
      }
      if (!solucion and !frontera.empty())
      {
         nodoActual = frontera.front();
         solucion = nodoActual.estado.f == fin.f and nodoActual.estado.c == fin.c;
      }
   }

   if (solucion)
   {
      return nodoActual.acciones;
   }
   return list<Action>({});
}


Orientacion ComportamientoAuxiliar::cambiarOrientacion(const Orientacion inicial)
{
   Orientacion final = norte;
   switch (inicial)
   {
   case norte:
      final = noreste;
      break;
   case noreste:
      final = este;
      break;
   case este:
      final = sureste;
      break;
   case sureste:
      final = sur;
      break;
   case sur:
      final = suroeste;
      break;
   case suroeste:
      final = oeste;
      break;
   case oeste:
      final = noroeste;
      break;
   case noroeste:
      final = norte;
      break;
   }

   return final;
}

Action ComportamientoAuxiliar::tomarDecisionNivel_0(const Sensores &s)
{
    if (s.superficie[0] == 'X')
        return IDLE;

    if (leftCounter > 0) {
        --leftCounter;
        return TURN_SR;
    }

    int mejorMov = -1, mejorDist = INT_MAX;
    for (int j = 1; j < 16; ++j) {
        if (s.superficie[j] != 'X') continue;
        DFC dX = calcularDiferencias(j, s.rumbo);
        for (int i : {1,2,3}) {
            int dh = abs(s.cota[0] - s.cota[i]);
            bool altOK = dh < 2 || (tieneZapatillas && dh < 3);
            if (!altOK || s.agentes[i] != '_') continue;
            DFC dI = calcularDiferencias(i, s.rumbo);
            int manh = abs(dX.df - dI.df) + abs(dX.dc - dI.dc);
            if (manh < mejorDist) {
                mejorDist = manh;
                mejorMov  = i;
            }
        }
    }
    if (mejorMov != -1) {
        switch (mejorMov) {
            case 2: 
                return WALK;
            case 1:
                leftCounter = 6;
                return TURN_SR;
            case 3:
                return TURN_SR;
        }
    }

    auto score = [&](int i){
        int dh = abs(s.cota[0] - s.cota[i]);
        bool altOK = dh < 2 || (tieneZapatillas && dh < 3);
        if (!altOK || s.agentes[i] != '_') return 0;
        char t = s.superficie[i];
        if (!tieneZapatillas && t == 'D') return 50;
        if (t == 'C')                     return 20;
        if (t == 'S')                     return 5;
        if (t == '?')                     return 1;
        return 0;
    };

    int pI = score(1), pF = score(2), pD = score(3);
    if (pF >= pI && pF >= pD && pF > 0)
        return WALK;
    if (pI > 0) {
        leftCounter = 6;
        return TURN_SR;
    }
    if (pD > 0)
        return TURN_SR;

    leftCounter = 6;
    return TURN_SR;
}

bool ComportamientoAuxiliar::casillaAccesibleAuxiliar(const EstadoA &st, const vector<vector<unsigned char>> &terreno, const vector<vector<unsigned char>> &altura)
{
   EstadoA siguiente = siguienteCasillaAuxiliar(st);
   bool check1 = terreno[siguiente.f][siguiente.c] != 'P' and terreno[siguiente.f][siguiente.c] != 'M';
   bool check2 = terreno[siguiente.f][siguiente.c] != 'B' or (terreno[siguiente.f][siguiente.c] == 'B' and st.zapatillas);
   bool check3 = abs(altura[siguiente.f][siguiente.c] - altura[st.f][st.c]) <= 1;
   return check1 and check2 and check3;
}

EstadoA ComportamientoAuxiliar::siguienteCasillaAuxiliar(const EstadoA &st)
{
   EstadoA siguiente = st;
   switch (st.brujula)
   {
   case norte:
      siguiente.f = st.f - 1;
      break;
   case noreste:
      siguiente.f = st.f - 1;
      siguiente.c = st.c + 1;
      break;
   case este:
      siguiente.c = st.c + 1;
      break;
   case sureste:
      siguiente.f = st.f + 1;
      siguiente.c = st.c + 1;
      break;
   case sur:
      siguiente.f = st.f + 1;
      break;
   case suroeste:
      siguiente.f = st.f + 1;
      siguiente.c = st.c - 1;
      break;
   case oeste:
      siguiente.c = st.c - 1;
      break;
   case noroeste:
      siguiente.f = st.f - 1;
      siguiente.c = st.c - 1;
      break;
   }

   return siguiente;
}


void ComportamientoAuxiliar::actualizarEstado(const Sensores &sensores)
{
   if (sensores.superficie[0] == 'D')
   {
      tieneZapatillas = true;
   }
   if (sensores.superficie[0] == 'K') {
      tieneBikini = true;
   }

   rellenarMapa(sensores, mapaResultado, true);
   rellenarMapa(sensores, mapaCotas, false);
}

int ComportamientoAuxiliar::interact(Action accion, int valor)
{
	return 0;
}

Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_0(Sensores sensores)
{
	actualizarEstado(sensores);
	return tomarDecisionNivel_0(sensores);
}

Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_1(Sensores sensores)
{
   actualizarEstado(sensores);
   return tomarDecisionNivel_1(sensores);
}


Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_E(Sensores sensores)
{
   Action action = IDLE;

   if (!hayPlan)
   {
      //plan = busquedaAnchura();
      hayPlan = true;
   }
   else if (hayPlan && !plan.empty())
   {
      action = plan.front();
      plan.pop_front();
   }
   else
   {
      hayPlan = false;
   }
   return action;
}

Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_2(Sensores sensores)
{
}

Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_3(Sensores sensores) {
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

Action ComportamientoAuxiliar::buscarAlternativa(const Sensores &sensores) {

   if (casillaAccesible(sensores, 2)) return WALK;
   if (casillaAccesible(sensores, 3)) return TURN_SR;
   if (casillaAccesible(sensores, 1)) return TURN_L;
   
   return TURN_SR;
}

std::pair<int, int> ComportamientoAuxiliar::getOffsetPosicion(int idx) {
   const int offsets[17][2] = {
       {0,0},
       {-1,1}, {0,1}, {1,1}, 
       {-1,0}, {1,0},
       {-1,-1}, {0,-1}, {1,-1},
       {-2,2}, {0,2}, {2,2},
       {-2,0}, {2,0},
       {-2,-2}, {0,-2}, {2,-2}
   };
   return {offsets[idx][0], offsets[idx][1]};
}

std::pair<int, int> ComportamientoAuxiliar::calcularPosicionAbsoluta(const Sensores& s, int idx) {
   auto offset = getOffsetPosicion(idx);
   int fil = s.posF + offset.first;
   int col = s.posC + offset.second;
   return {fil, col};
}

bool ComportamientoAuxiliar::yaDescubierto(const Sensores& s, int idx) {
   auto pos = calcularPosicionAbsoluta(s, idx);
   return celdasMapeadas.count(pos) > 0;
}

void ComportamientoAuxiliar::actualizarMapa(const Sensores& s) {
   for (int i = 0; i < s.superficie.size(); ++i) {
       if (s.superficie[i] == 'C' || s.superficie[i] == 'S') {
           auto pos = calcularPosicionAbsoluta(s, i);
           celdasMapeadas.insert(pos);
           
           mapaResultado[pos.first][pos.second] = s.superficie[i];
       }
   }
}

list<Action> ComportamientoAuxiliar::aEstrella(
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

vector<pair<Action, EstadoA>> ComportamientoAuxiliar::generarVecinosAStar(
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

int ComportamientoAuxiliar::heuristica(const EstadoA &actual, const EstadoA &objetivo) {
   return (abs(actual.f - objetivo.f) + abs(actual.c - objetivo.c)) * 10;
}

bool ComportamientoAuxiliar::casillaAccesible(
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

list<Action> ComportamientoAuxiliar::reconstruirCamino(
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

int ComportamientoAuxiliar::calcularCoste(
   const EstadoA &actual, 
   const EstadoA &siguiente, 
   const vector<vector<unsigned char>> &mapa,
   const vector<vector<unsigned char>> &alturas) {
   
   char terreno = mapa[siguiente.f][siguiente.c];
   int coste = 0;

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

EstadoA ComportamientoAuxiliar::siguienteCasilla(const EstadoA &actual) {
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

Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_4(Sensores sensores)
{
   actualizarEstado(sensores);

   // Se não foi chamado, fica parado
   if (!sensores.venpaca) {
       return IDLE;
   }

   // Se já chegou ao destino, para
   if (sensores.posF == sensores.destinoF && sensores.posC == sensores.destinoC) {
       plan.clear();
       return IDLE;
   }

   // Se tem plano, segue
   if (!plan.empty()) {
       Action accion = plan.front();
       plan.pop_front();
       return accion;
   }

   // Caso contrário, cria plano até a posição indicada
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

   if (!plan.empty()) {
       Action accion = plan.front();
       plan.pop_front();
       return accion;
   }

   // Se tudo falhar, tenta se mover de forma simples
   return buscarAlternativa(sensores);
}