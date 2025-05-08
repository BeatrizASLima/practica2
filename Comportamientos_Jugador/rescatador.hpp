#ifndef COMPORTAMIENTORESCATADOR_H
#define COMPORTAMIENTORESCATADOR_H

#include <chrono>
#include <cstdlib>
#include <deque>
#include <functional>
#include <list>
#include <set>
#include <time.h>
#include <thread>

#include "auxiliar.hpp"
#include "comportamientos/comportamiento.hpp"

/**
 * estructura para almacenar diferencias de filas y columnas
 */
struct DFC{
   int df;
   int dc;
};


/**
 * diferencia de filas y columnas para N, S, E, O
 */
static vector<DFC> difBase{
        {0, 0},
        {-1, -1},
        {-1, 0},
        {-1, 1},
        {-2, -2},
        {-2, -1},
        {-2, 0},
        {-2, 1},
        {-2, 2},
        {-3, -3},
        {-3, -2},
        {-3, -1},
        {-3, 0},
        {-3, 1},
        {-3, 2},
        {-3,2}
};

/**
 * diferencias de filas y columnas para NE, NO, SE, SO
 */
static vector<DFC> difCombinadas = {
	{0, 0},
	{-1, 0},
	{-1, 1},
	{0, 1},
	{-2, 0},
	{-2, 1},
	{-2, 2},
	{-1, 2},
	{0, 2},
	{-3, 0},
	{-3, 1},
	{-3, 2},
	{-3, 3},
	{-2, 3},
	{-1, 3},
	{0, 3}
};

/**
 * metodo estatico para calcular la diferencia con respecto a la posicion
 * inicial del agente para un determinado indice, relacionado con el vector
 * de sensores
 * @param indice
 * @param orientacion
 * @return
 */
static DFC calcularDiferencias(int indice, Orientacion orientacion){
   DFC resultado = {0, 0};

   // se calcula en funcion de la orientacion
   switch(orientacion){
      case norte:
         // para la orientacion N se copia directamente la estructura
         // de la posicion 0 
         resultado = difBase[indice];
         break;
      case sur:
         resultado.df = difBase[indice].df * (-1);
         resultado.dc = difBase[indice].dc * (-1);
         break;
      case este:
         resultado.df = difBase[indice].dc;
         resultado.dc = difBase[indice].df * (-1);
         break;
      case oeste:
         resultado.df = difBase[indice].dc * (-1);
         resultado.dc = difBase[indice].df;
         break;
      case noreste:
         // para la orientacion NE se copia la estructura de la
         // posicion 0 de difCombinadas
         resultado = difCombinadas[indice];
         break;
      case sureste:
         resultado.df = difCombinadas[indice].dc;
         resultado.dc = difCombinadas[indice].df * (-1);
         break;
      case noroeste:
         resultado.df = difCombinadas[indice].dc * (-1);
         resultado.dc = difCombinadas[indice].df;
         break;
      case suroeste:
         resultado.df = difCombinadas[indice].df * (-1);
         resultado.dc = difCombinadas[indice].dc * (-1);
         break;
   }

   // se devuelve el resultado
   return resultado;
}

/**
 * metodo para rellenar el mapa de resultado o el de cotas
 * @param sensores
 * @param mapa
 * @param superficie flag booleano para indicar si se rellena el
 *                   mapa de superficie o el de cota
 */
static void rellenarMapa(const Sensores & sensores, vector< vector< unsigned char> > & mapa,
                         bool superficie){
   // se consideran las 16 posiciones del vector de informacion
   int filaDest = 0, columnaDest = 0;

   // se asume inicialmente que el vector del que copiar es el de superficie
   vector<unsigned char> vectorInfo = sensores.superficie;

   // en caso contrario, se reasigna al de cotas
   if(!superficie){
      vectorInfo = sensores.cota;
   }

   // se caclulan las diferencias y se hace la copia sobre el mapa
   // pasado como argumento
   DFC diferencias;
   for(int i=0; i < 16; i++){
      diferencias = calcularDiferencias(i, sensores.rumbo);
      filaDest = sensores.posF + diferencias.df;
      columnaDest = sensores.posC + diferencias.dc;
      mapa[filaDest][columnaDest] = vectorInfo[i];
   }
}

class ComportamientoRescatador : public Comportamiento {

public:
  ComportamientoRescatador(unsigned int size = 0) : Comportamiento(size){
  }

  ComportamientoRescatador(std::vector<std::vector<unsigned char>> mapaR, std::vector<std::vector<unsigned char>> mapaC) : Comportamiento(mapaR,mapaC)
  {
    // Inicializar Variables de Estado Niveles 2,3
  }
  ComportamientoRescatador(const ComportamientoRescatador &comport) : Comportamiento(comport) {}
  ~ComportamientoRescatador() {}

  Action think(Sensores sensores);

  int interact(Action accion, int valor);

  Action ComportamientoRescatadorNivel_0(Sensores sensores);
  Action ComportamientoRescatadorNivel_1(Sensores sensores);
  Action ComportamientoRescatadorNivel_2(Sensores sensores);
  Action ComportamientoRescatadorNivel_3(Sensores sensores);
  Action ComportamientoRescatadorNivel_4(Sensores sensores);

private:
   Action ultimaAccion = IDLE;
   bool tieneZapatillas = false;
   bool tieneBikini = false;
   int leftCounter = 0;
   int giroPendiente = 0;
   int lastF = -1;
   int lastC = -1;
   int stuck2 = 0;
    mutable int ultimaPosicionF = -1;
    mutable int ultimaPosicionC = -1;
    mutable int ultimaOrientacion = -1;

   vector<vector<bool>> mapaVisitado;
    vector<vector<bool>> mapaObstaculos;
    bool mapaInicializado = false;
    
    list<Action> plan;
    int intentosReplanificacion = 0;
    const int MAX_REPLAN = 5;

   bool hayPlan = false;

   void actualizarEstado(const Sensores &sensores);
   Action tomarDecisionNivel_0(const Sensores &sensores);
   Action tomarDecisionNivel_1(const Sensores &sensores);
   int indiceCasillaMasInteresante(const Sensores &sensores);
   char viablePorAltura(int indice, const Sensores &sensores);
   bool caminoViable(const Sensores &s, int idx) const;
   int scoreCasilla(const Sensores& s, int i);
   bool casillaAccesible(const Sensores& s, int idx) const;


   // 2

   void calcularCaminoDijkstra(const Sensores &sensores);
   bool estaEnCaminoEsperado(const Sensores &sensores) const;
   bool esTerrenoTransitable(char terreno) const;
   Action buscarAlternativa(const Sensores &sensores);
   bool esCasillaAccesible(int fila, int columna, const Sensores &sensores) const;
   bool esTerrenoTransitableCompleto(char terreno, int desnivel) const;
   void inicializarMapaGlobal(int filas, int columnas);
   //3
   list<Action> aEstrella(
      const EstadoA &inicio, 
      const EstadoA &objetivo,
      const vector<vector<unsigned char>> &mapa,
      const vector<vector<unsigned char>> &alturas);
   
   vector<pair<Action, EstadoA>> generarVecinosAStar(
         const EstadoA &actual,
         const vector<vector<unsigned char>> &mapa,
         const vector<vector<unsigned char>> &alturas);
         
   int heuristica(const EstadoA &actual, const EstadoA &objetivo);
   
   bool casillaAccesible(
      const EstadoA &actual,
      const EstadoA &siguiente,
      const vector<vector<unsigned char>> &mapa,
      const vector<vector<unsigned char>> &alturas);
   int getCostoTerreno(char terreno) const;

   list<Action> reconstruirCamino(const EstadoA& inicio, const EstadoA& objetivo,
   const unordered_map<string, EstadoA>& padre,
   const unordered_map<string, Action>& accion_padre,
   function<string(const EstadoA&)> generarClave);

   int calcularCoste(const EstadoA& actual, const EstadoA& siguiente,
   const vector<vector<unsigned char>>& mapa,
   const vector<vector<unsigned char>>& alturas);

   EstadoA siguienteCasilla(const EstadoA& actual);

  void convertirCaminoAAcciones(const vector<pair<int, int>>& camino, const Sensores &sensores);


   std::set<std::pair<int, int>> celdasVisitadas; 

    std::set<std::pair<int, int>> celdasCyS;

    bool yaDescubierto(const Sensores& s, int idx);
};

#endif