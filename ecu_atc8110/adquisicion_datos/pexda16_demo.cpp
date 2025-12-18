#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

// ------------------------------------------------------------
// 1) MODO DRIVER (/dev/...) vía IOCTL
// ------------------------------------------------------------
// IMPORTANTE:
// - Lo ideal es incluir el header oficial del driver/SDK (p.ej. pexda16_ioctl.h).
// - Si lo tienes, define HAVE_PEXDA16_IOCTL_H y pon el include correcto.
//
// Ejemplo:
//   g++ -O2 -std=c++17 pexda16_demo.cpp -DHAVE_PEXDA16_IOCTL_H -o pexda16_demo
//
// y dentro:
//   #include "pexda16_ioctl.h"
// ------------------------------------------------------------
#ifdef HAVE_PEXDA16_IOCTL_H
#include "pexda16_ioctl.h"
#else
// Fallback: definiciones PLACEHOLDER (NO GARANTIZAN coincidir con tu driver).
// Úsalas solo si no tienes el header, para compilar y que el programa te diga qué falta.
struct pexda16_ao_write_t { uint32_t channel; int32_t value; };  // value: código DAC o mV según driver
struct pexda16_di_read_t  { uint32_t port;    uint32_t value; };

#ifndef _IOC
#include <linux/ioctl.h>
#endif

#define PEXDA16_IOC_MAGIC 'p'
// NOTA: estos números son orientativos. Debes sustituirlos por los reales.
#define PEXDA16_IOCTL_AO_WRITE _IOW(PEXDA16_IOC_MAGIC, 1, pexda16_ao_write_t)
#define PEXDA16_IOCTL_DI_READ  _IOWR(PEXDA16_IOC_MAGIC, 2, pexda16_di_read_t)
#endif

static std::string errstr() {
  return std::string(std::strerror(errno)) + " (errno=" + std::to_string(errno) + ")";
}

static int open_dev(const std::string& dev) {
  int fd = ::open(dev.c_str(), O_RDWR | O_CLOEXEC);
  if (fd < 0) {
    std::cerr << "[open] " << dev << " -> " << errstr() << "\n";
  }
  return fd;
}

// Escala simple: Voltios -> código 16-bit unsigned (0..65535) para rango 0..10V.
// Si tu rango es distinto, ajusta minV/maxV.
static int32_t volts_to_u16(double v, double minV=0.0, double maxV=10.0) {
  if (v < minV) v = minV;
  if (v > maxV) v = maxV;
  double norm = (v - minV) / (maxV - minV); // 0..1
  int32_t code = static_cast<int32_t>(norm * 65535.0 + 0.5);
  if (code < 0) code = 0;
  if (code > 65535) code = 65535;
  return code;
}

static bool ao_write_ioctl(int fd, uint32_t ch, double volts) {
  pexda16_ao_write_t req{};
  req.channel = ch;
  req.value   = volts_to_u16(volts); // OJO: algunos drivers esperan mV o signed; ajusta si aplica.

  if (::ioctl(fd, PEXDA16_IOCTL_AO_WRITE, &req) < 0) {
    std::cerr << "[AO ioctl] ch=" << ch << " volts=" << volts << " -> " << errstr() << "\n";
    return false;
  }
  std::cout << "[AO ioctl] OK ch=" << ch << " volts=" << volts << " code=" << req.value << "\n";
  return true;
}

static bool di_read_ioctl(int fd, uint32_t port, uint32_t& out) {
  pexda16_di_read_t req{};
  req.port = port;

  if (::ioctl(fd, PEXDA16_IOCTL_DI_READ, &req) < 0) {
    std::cerr << "[DI ioctl] port=" << port << " -> " << errstr() << "\n";
    return false;
  }
  out = req.value;
  std::cout << "[DI ioctl] OK port=" << port << " value=" << out << "\n";
  return true;
}

// ------------------------------------------------------------
// 2) MODO ACCESO DIRECTO A PUERTOS I/O (requiere CAP_SYS_RAWIO o root)
// ------------------------------------------------------------
#if defined(__linux__)
#include <sys/io.h>
static bool enable_io(uint16_t base, uint16_t len) {
  // ioperm / iopl requieren permisos elevados; si falla verás EPERM.
  if (ioperm(base, len, 1) != 0) {
    std::cerr << "[ioperm] base=0x" << std::hex << base << " len=" << std::dec << len
              << " -> " << errstr() << "\n";
    return false;
  }
  return true;
}
static void disable_io(uint16_t base, uint16_t len) { ioperm(base, len, 0); }
#endif

int main(int argc, char** argv) {
  std::string dev = "/dev/pexda16_0"; // cambia si tu nodo es distinto
  bool doAO = false, doDI = false;
  uint32_t aoCh = 0, diPort = 0;
  double aoVolts = 0.5;

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--dev" && i + 1 < argc) dev = argv[++i];
    else if (a == "--ao" && i + 2 < argc) { doAO = true; aoCh = std::stoul(argv[++i]); aoVolts = std::stod(argv[++i]); }
    else if (a == "--di" && i + 1 < argc) { doDI = true; diPort = std::stoul(argv[++i]); }
    else if (a == "--help") {
      std::cout << "Uso:\n"
                << "  " << argv[0] << " --dev /dev/pexda16_0 --ao <ch> <volts>\n"
                << "  " << argv[0] << " --dev /dev/pexda16_0 --di <port>\n";
      return 0;
    }
  }

  int fd = open_dev(dev);
  if (fd < 0) {
    std::cerr << "\nSugerencias rápidas:\n"
              << "  - Comprueba el nodo: ls -l /dev/pexda16*\n"
              << "  - Si ves Permission denied: aplica udev o ejecuta con sudo.\n";
    return 2;
  }

  if (doAO) {
    // Si aquí te da EPERM (Operation not permitted), casi siempre es permisos (udev/capabilities/sudo).
    ao_write_ioctl(fd, aoCh, aoVolts);
  }

  if (doDI) {
    uint32_t v = 0;
    // Si aquí te da EINVAL (Invalid argument), normalmente es:
    //  - port fuera de rango
    //  - ioctl equivocado / estructura distinta a la esperada por el driver
    //  - estás usando definiciones placeholder en vez del header real
    di_read_ioctl(fd, diPort, v);
  }

  ::close(fd);
  return 0;
}
