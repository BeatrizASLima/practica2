#ifndef COMPORTAMIENTOAUXILIAR_H
#define COMPORTAMIENTOAUXILIAR_H

#include <chrono>
#include <functional>
#include <list>
#include <set>
#include <string>
#include <unordered_map>
#include "comportamientos/comportamiento.hpp"


struct EstadoA {
  int f;
  int c;
  Orientacion brujula;
  bool zapatillas;
  bool bikini;

  bool operator==(const EstadoA & otro) const {
    return (f == otro.f && c == otro.c && brujula == otro.brujula && zapatillas == otro.zapatillas);
  }
};

struct NodoA {
  EstadoA estado;
  list<Action> acciones;

  bool operator==(const NodoA & otro) const {
    return (estado == otro.estado);
  }
};


struct NodoAStar {
  EstadoA estado;
  int g;
  int h;
  int f;
  
  bool operator>(const NodoAStar& otro) const {
      return f > otro.f;
  }
};

struct NodoDijkstra {
  EstadoA estado;
  int coste;
  bool operator>(const NodoDijkstra& other) const { return coste > other.coste; }
};


class ComportamientoAuxiliar : public Comportamiento
{

public:
  ComportamientoAuxiliar(unsigned int size = 0) : Comportamiento(size) {
    // Inicializar Variables de Estado Niveles 0,1,4
    ultimaAccion = IDLE;
    tieneZapatillas = false;
    hayPlan = false;
    giro45Izqda = 0;
    giros = 0;
    leftCounter = 0;
  }

  ComportamientoAuxiliar(std::vector<std::vector<unsigned char>> mapaR, std::vector<std::vector<unsigned char>> mapaC) : Comportamiento(mapaR,mapaC) {
    hayPlan = false;
    tieneZapatillas = false;
  }
  ComportamientoAuxiliar(const ComportamientoAuxiliar &comport) : Comportamiento(comport) {}
  ~ComportamientoAuxiliar() {}

  Action think(Sensores sensores);

  int interact(Action accion, int valor);

  Action ComportamientoAuxiliarNivel_0(Sensores sensores);
  Action ComportamientoAuxiliarNivel_1(Sensores sensores);
  Action ComportamientoAuxiliarNivel_2(Sensores sensores);
  Action ComportamientoAuxiliarNivel_E(Sensores sensores);
  Action ComportamientoAuxiliarNivel_3(Sensores sensores);
  Action ComportamientoAuxiliarNivel_4(Sensores sensores);

private:
  Action ultimaAccion   = IDLE; 
  int    girosPendientes = 0; 
  int    girosSeguidos   = 0; 
  int    leftCounter = 0;


  list<Action> plan;

  bool hayPlan;


  bool tieneZapatillas;
  bool tieneBikini = false;
  int giro45Izqda;
  int giros;

  /**
    * metodo para actualizar las variables de estado
    */
  void actualizarEstado(const Sensores & sensores);

  /**
    * metodo de toma de decisiones para el nivel0 (version 0)
    * @param sensores
    */
  Action tomarDecisionNivel_0(const Sensores & sensores);

  /**
  * metodo para determinar que casilla es la mas interesante: la
  * izquierda, central o derecha
  * @param izqda
  * @param frente
  * @param dcha
  * @return
  */
  int indiceCasillaMasInteresante(const Sensores & sensores);

  /**
    * determina si una casilla de las accesibles (indice) es viable
    * por altura: si lo es devuelve su valor y en caso controario
    * devolvera P
    * @param indice
    * @param sensores
    * @return
    */
  char viablePorAltura(int indice, const Sensores & sensores);


  list<Action> avanzaSaltosDeCaballo();
  list<Action> busquedaAnchura(const EstadoA &inicio, const EstadoA &fin, const vector<vector<unsigned char>> &terreno, const vector<vector<unsigned char>> &altura);

  EstadoA siguienteCasillaAuxiliar(const EstadoA &st);
  bool casillaAccesibleAuxiliar(const EstadoA &st, const vector<vector<unsigned char>> &terreno, const vector<vector<unsigned char>> &altura);
  bool encontrar(const NodoA &st, const list<NodoA> &lista);
  Orientacion cambiarOrientacion(const Orientacion inicial);
  EstadoA aplicar(Action accion, const EstadoA &st, const vector<vector<unsigned char>> &terreno, const vector<vector<unsigned char>> &altura);
  void visualizarPlan(const EstadoA &st, const list<Action> &plan);
  void anularMatriz(vector<vector<unsigned char>> &matriz);


  bool casillaAccesible(const Sensores& s, int idx) const;
  Action tomarDecisionNivel_1(const Sensores& s);



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
  Action buscarAlternativa(const Sensores &sensores);
  EstadoA siguienteCasilla(const EstadoA &actual);




  std::set<std::pair<int, int>> celdasMapeadas;


    std::pair<int, int> getOffsetPosicion(int idx);

    std::pair<int, int> calcularPosicionAbsoluta(const Sensores& s, int idx);

    bool yaDescubierto(const Sensores& s, int idx);

    void actualizarMapa(const Sensores& s);
};

#endif
