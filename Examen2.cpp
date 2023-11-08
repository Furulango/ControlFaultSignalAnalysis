#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <queue>
#include <utility>
#include <fstream>
#include <filesystem>

// Estructura para representar una señal y su etiqueta (0 para control, 1 para falla).
class Signal {
public:
    std::vector<double> valores;
    int bandera; // 0 para señal de control, 1 para señal de falla

    Signal(std::vector<double> vals, int flag) : valores(vals), bandera(flag) {}

    // Constructor por defecto si se requiere
    Signal() : bandera(0) {}

    // Otros métodos y miembros si son necesarios
};

// Función para calcular el factor de impulso, que es el promedio de los valores de la señal.
double calcularFactorImpulso(const std::vector<double>& datos) {
    double suma = 0.0;
    for (double valor : datos) {
        suma += valor;
    }
    return datos.size() > 0 ? suma / datos.size() : 0.0; // Evitar división por cero.
}

// Función para calcular el factor de latitud, que es la raíz cuadrada de la suma de los cuadrados de los valores de la señal.
double calcularFactorLatitud(const std::vector<double>& datos) {
    double suma = 0.0;
    for (double valor : datos) {
        suma += valor * valor;
    }
    return std::sqrt(suma);
}

// Función para procesar las señales y guardar los resultados
void procesarYGuardarSeñales(const std::vector<Signal>& señales, const std::string& archivoSalida) {
    std::ofstream salida(archivoSalida, std::ios::out);

    if (!salida) {
        std::cerr << "Error al abrir el archivo de salida." << std::endl;
        return;
    }

    for (const auto& señal : señales) {
        double factorImpulso = calcularFactorImpulso(señal.valores);
        double factorLatitud = calcularFactorLatitud(señal.valores);

        // Guarda los factores calculados y la bandera en el archivo de salida con el formato solicitado
        salida << "Factor de Impulso: " << factorImpulso
               << " Factor de Latitud: " << factorLatitud
               << " Etiqueta: " << señal.bandera << std::endl;
    }

    salida.close();
}

// Función para leer los datos de un archivo y almacenarlos en una estructura de señal.
Signal leerDatos(const std::string& archivoNombre, int bandera) {

    std::ifstream archivo(archivoNombre);
    if (!archivo) {
        throw std::runtime_error("No se puede abrir el archivo: " + archivoNombre);
    }

    std::vector<double> valores;
    double valor;
    while (archivo >> valor) {
        valores.push_back(valor);
    }
    return Signal(valores, bandera);
}

double calcularDistanciaEuclidiana(const Signal& a, const Signal& b) {
    double distancia = 0.0;
    for (size_t i = 0; i < a.valores.size() && i < b.valores.size(); ++i) {
        distancia += std::pow(a.valores[i] - b.valores[i], 2);
    }
    return std::sqrt(distancia);
}

// Función para predecir la clase de una nueva señal utilizando KNN.
int predecirKNN(const std::vector<Signal>& entrenamiento, const Signal& nuevoPunto, int k) {
    // Usaremos una cola de prioridad para mantener las k señales más cercanas.
    auto comp = [](const std::pair<double, int>& a, const std::pair<double, int>& b) {
        return a.first > b.first;
    };
    std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, decltype(comp)> pq(comp);

    // Calculamos la distancia del nuevoPunto a todas las señales de entrenamiento.
    for (const auto& señal : entrenamiento) {
        double distancia = calcularDistanciaEuclidiana(nuevoPunto, señal);
        pq.push({distancia, señal.bandera});
        if (pq.size() > k) {
            pq.pop();
        }
    }

    // Contamos la frecuencia de las etiquetas de las k señales más cercanas.
    std::vector<int> cuentaClases(2, 0); // Si solo tienes dos clases: control(0) y falla(1).
    while (!pq.empty()) {
        cuentaClases[pq.top().second]++;
        pq.pop();
    }

    // La clase con la mayor frecuencia es nuestra predicción.
    return cuentaClases[1] > cuentaClases[0] ? 1 : 0;
}

// Función para calcular la precisión del modelo KNN sobre el conjunto de validación.
std::pair<double, std::vector<int>> calcularPrecisionYPredicciones(const std::vector<Signal>& validacion, const std::vector<Signal>& entrenamiento, int k) {
    int aciertos = 0;
    std::vector<int> predicciones;
    for (const auto& señal : validacion) {
        int predicha = predecirKNN(entrenamiento, señal, k);
        predicciones.push_back(predicha);
        if (predicha == señal.bandera) {
            aciertos++;
        }
    }
    double precision = 100.0 * aciertos / validacion.size();
    return {precision, predicciones};
}

void escribirResultadosYPrecision(const std::vector<Signal>& validacion, const std::vector<int>& predicciones, double precision, const std::string& archivoResultados) {
    std::ofstream salida(archivoResultados, std::ios::out);
    if (!salida) {
        std::cerr << "Error al abrir el archivo de resultados: " << archivoResultados << std::endl;
        return;
    }
    
    for (size_t i = 0; i < validacion.size(); ++i) {
        double factorImpulso = calcularFactorImpulso(validacion[i].valores);
        double factorLatitud = calcularFactorLatitud(validacion[i].valores);
        salida << "Factor de Impulso: " << factorImpulso
               << " Factor de Latitud: " << factorLatitud
               << ", Etiqueta predicha: " << predicciones[i] << std::endl;
        std::cout << "Archivo: " << i << " es: " << predicciones[i] << std::endl;
    }
    std::cout << " donde: (0 falla, 1 control)" << std::endl;
    
    salida.close();
}

// MAIN 

int main() {
    std::vector<Signal> señalesControl;
    std::vector<Signal> señalesFalla;
    int numeroDeArchivosControl, numeroDeArchivosFalla;
    std::string nombreBaseControl, nombreBaseFalla, extensionControl, extensionFalla;

    // Solicitar al usuario el número de archivos de control, el nombre base y su extensión
    std::cout << "Ingrese el número de archivos de control: ";
    std::cin >> numeroDeArchivosControl;
    std::cout << "Ingrese el nombre base para los archivos de control: ";
    std::cin >> nombreBaseControl;
    std::cout << "Ingrese la extensión de los archivos de control (incluya el punto): ";
    std::cin >> extensionControl;

    // Leer los archivos de control
    for (int i = 1; i <= numeroDeArchivosControl; ++i) {
        std::string nombreArchivoControl = nombreBaseControl + std::to_string(i) + extensionControl;
        try {
            señalesControl.push_back(leerDatos(nombreArchivoControl, 0));
        } catch (const std::exception& e) {
            std::cerr << "Error al leer el archivo de control " << nombreArchivoControl << ": " << e.what() << std::endl;
        }
    }

    // Solicitar al usuario el número de archivos de falla, el nombre base y su extensión
    std::cout << "Ingrese el número de archivos de falla: ";
    std::cin >> numeroDeArchivosFalla;
    std::cout << "Ingrese el nombre base para los archivos de falla: ";
    std::cin >> nombreBaseFalla;
    std::cout << "Ingrese la extensión de los archivos de falla (incluya el punto): ";
    std::cin >> extensionFalla;

    // Leer los archivos de falla
    for (int i = 1; i <= numeroDeArchivosFalla; ++i) {
        std::string nombreArchivoFalla = nombreBaseFalla + std::to_string(i) + extensionFalla;
        try {
            señalesFalla.push_back(leerDatos(nombreArchivoFalla, 1));
        } catch (const std::exception& e) {
            std::cerr << "Error al leer el archivo de falla " << nombreArchivoFalla << ": " << e.what() << std::endl;
        }
    }

    std::vector<Signal> datosEntrenamiento;
    datosEntrenamiento.insert(datosEntrenamiento.end(), señalesControl.begin(), señalesControl.end());
    datosEntrenamiento.insert(datosEntrenamiento.end(), señalesFalla.begin(), señalesFalla.end());

    // Procesar y guardar las señales de entrenamiento
    procesarYGuardarSeñales(datosEntrenamiento, "entrenamiento.csv");

    // Pedir al usuario archivos de validación
    std::vector<Signal> datosValidacion;
    int numeroDeArchivosValidacion;
    std::string nombreBaseValidacion, extensionValidacion;
    
    std::cout << "Ingrese el número de archivos de validación: ";
    std::cin >> numeroDeArchivosValidacion;
    std::cout << "Ingrese el nombre base para los archivos de validación: ";
    std::cin >> nombreBaseValidacion;
    std::cout << "Ingrese la extensión de los archivos de validación (incluya el punto): ";
    std::cin >> extensionValidacion;

    // Leer los archivos de validación
    for (int i = 1; i <= numeroDeArchivosValidacion; ++i) {
        std::string nombreArchivoValidacion = nombreBaseValidacion + std::to_string(i) + extensionValidacion;
        try {
            int etiquetaValidacion = 0;
            datosValidacion.push_back(leerDatos(nombreArchivoValidacion, etiquetaValidacion));
        } catch (const std::exception& e) {
            std::cerr << "Error al leer el archivo de validación " << nombreArchivoValidacion << ": " << e.what() << std::endl;
        }
    }

    // Establecer el valor de k para KNN
    int k = 3; // k vecinos más cercanos
    auto [precision, predicciones] = calcularPrecisionYPredicciones(datosValidacion, datosEntrenamiento, k);

    // Guardar la clasificación con la precisión en un archivo de texto
    escribirResultadosYPrecision(datosValidacion, predicciones, precision, "resultados_validacion.txt");

    return 0;
}