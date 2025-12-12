#include "../adquisicion_datos/sensor_manager.hpp"
#include "../adquisicion_datos/pex_device.hpp"
#include "../common/logging.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <map>

using namespace adquisicion_datos;
using namespace common;

class SensorDiagnostic {
public:
    SensorDiagnostic() : sensor_manager_() {}

    void run() {
        std::cout << "=== DIAGNÓSTICO DE SENSORES ECU FOX ===" << std::endl;
        std::cout << "Iniciando sensor manager..." << std::endl;
        
        if (!sensor_manager_.start()) {
            std::cerr << "ERROR: No se pudo iniciar el sensor manager" << std::endl;
            return;
        }

        // Verificar estado del dispositivo PEX
        int fd = PexDevice::GetInstance().GetFd();
        if (fd < 0) {
            std::cerr << "ERROR: Dispositivo PEX no disponible (FD: " << fd << ")" << std::endl;
            std::cerr << "Verifique que el driver esté cargado y tenga permisos." << std::endl;
            return;
        }
        std::cout << "✓ Dispositivo PEX conectado (FD: " << fd << ")" << std::endl << std::endl;

        // Estadísticas de lectura
        std::map<std::string, SensorStats> stats;
        int iteration = 0;
        const int max_iterations = 100; // 100 lecturas = ~5 segundos

        std::cout << "Leyendo sensores..." << std::endl;
        std::cout << "----------------------------------------------------------------------" << std::endl;

        while (iteration < max_iterations) {
            auto samples = sensor_manager_.poll();
            
            // Actualizar estadísticas
            for (const auto& sample : samples) {
                auto& s = stats[sample.name];
                s.update(sample.value);
            }

            // Mostrar cada 10 iteraciones
            if (iteration % 10 == 0) {
                print_status(iteration, samples, stats);
            }

            iteration++;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::cout << "\n======================================================================" << std::endl;
        std::cout << "RESUMEN FINAL DEL DIAGNÓSTICO" << std::endl;
        std::cout << "======================================================================" << std::endl;
        print_summary(stats);
        
        sensor_manager_.stop();
    }

private:
    struct SensorStats {
        double min_val = 1e9;
        double max_val = -1e9;
        double sum = 0;
        int count = 0;
        int zero_count = 0;
        
        void update(double val) {
            if (val < min_val) min_val = val;
            if (val > max_val) max_val = val;
            sum += val;
            count++;
            if (val == 0.0) zero_count++;
        }
        
        double avg() const { return count > 0 ? sum / count : 0; }
        bool is_stuck() const { return min_val == max_val; }
        bool is_all_zero() const { return zero_count == count; }
    };

    void print_status(int iter, const std::vector<AnalogSample>& samples, 
                     const std::map<std::string, SensorStats>& stats) {
        std::cout << "\n[Iteración " << std::setw(3) << iter << "]" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        
        for (const auto& sample : samples) {
            auto it = stats.find(sample.name);
            if (it != stats.end()) {
                const auto& s = it->second;
                std::cout << "  " << std::setw(15) << std::left << sample.name 
                         << ": " << std::setw(8) << std::right << sample.value
                         << " | Min: " << std::setw(8) << s.min_val
                         << " | Max: " << std::setw(8) << s.max_val
                         << " | Avg: " << std::setw(8) << s.avg();
                
                // Indicadores de estado
                if (s.is_all_zero()) {
                    std::cout << " [⚠ ALL ZERO]";
                } else if (s.is_stuck()) {
                    std::cout << " [⚠ STUCK]";
                }
                std::cout << std::endl;
            }
        }
    }

    void print_summary(const std::map<std::string, SensorStats>& stats) {
        std::cout << std::fixed << std::setprecision(2);
        
        bool all_ok = true;
        
        for (const auto& [name, s] : stats) {
            std::cout << "\n📊 Sensor: " << name << std::endl;
            std::cout << "   Lecturas:  " << s.count << std::endl;
            std::cout << "   Mínimo:    " << s.min_val << std::endl;
            std::cout << "   Máximo:    " << s.max_val << std::endl;
            std::cout << "   Promedio:  " << s.avg() << std::endl;
            std::cout << "   Ceros:     " << s.zero_count << " (" 
                     << (s.count > 0 ? (100.0 * s.zero_count / s.count) : 0) << "%)" << std::endl;
            
            // Diagnóstico
            std::cout << "   Estado:    ";
            if (s.is_all_zero()) {
                std::cout << "❌ FALLO - Todas las lecturas son 0" << std::endl;
                std::cout << "   Posibles causas:" << std::endl;
                std::cout << "     - Sensor no conectado" << std::endl;
                std::cout << "     - Cable dañado" << std::endl;
                std::cout << "     - Canal ADC incorrecto" << std::endl;
                all_ok = false;
            } else if (s.is_stuck()) {
                std::cout << "⚠️  ADVERTENCIA - Valor constante (" << s.min_val << ")" << std::endl;
                std::cout << "   Posibles causas:" << std::endl;
                std::cout << "     - Sensor trabado mecánicamente" << std::endl;
                std::cout << "     - Sensor dañado (salida fija)" << std::endl;
                std::cout << "     - Falta de calibración" << std::endl;
                all_ok = false;
            } else if (s.zero_count > s.count * 0.8) {
                std::cout << "⚠️  ADVERTENCIA - Muchas lecturas en 0 (" 
                         << (100.0 * s.zero_count / s.count) << "%)" << std::endl;
                std::cout << "   Posibles causas:" << std::endl;
                std::cout << "     - Conexión intermitente" << std::endl;
                std::cout << "     - Ruido eléctrico" << std::endl;
                all_ok = false;
            } else {
                std::cout << "✅ OK - Sensor funcionando correctamente" << std::endl;
            }
        }
        
        std::cout << "\n======================================================================" << std::endl;
        if (all_ok) {
            std::cout << "✅ TODOS LOS SENSORES FUNCIONAN CORRECTAMENTE" << std::endl;
        } else {
            std::cout << "❌ SE DETECTARON PROBLEMAS EN UNO O MÁS SENSORES" << std::endl;
            std::cout << "\nACCIONES RECOMENDADAS:" << std::endl;
            std::cout << "1. Verificar conexiones físicas de los sensores fallidos" << std::endl;
            std::cout << "2. Revisar cableado y conectores" << std::endl;
            std::cout << "3. Verificar alimentación de sensores (5V/12V)" << std::endl;
            std::cout << "4. Comprobar configuración de canales en sensor_manager.cpp" << std::endl;
            std::cout << "5. Verificar calibración y escala de sensores" << std::endl;
        }
        std::cout << "======================================================================" << std::endl;
    }

    SensorManager sensor_manager_;
};

int main() {
    try {
        SensorDiagnostic diagnostic;
        diagnostic.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Excepción: " << e.what() << std::endl;
        return 1;
    }
}
