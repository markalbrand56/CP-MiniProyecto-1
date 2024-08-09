# Instrucciones para Ejecutar la Simulación del Ecosistema

## Requisitos

1. **Compilador de C con soporte para OpenMP**:
   - Asegúrate de tener un compilador que soporte OpenMP, como `gcc` o `clang`.

2. **Librerías necesarias**:
   - `pthread.h`: Generalmente incluida con el compilador, no se requiere instalación adicional.

3. **Sistema operativo**:
   - El código debería funcionar en la mayoría de los sistemas operativos basados en Unix (Linux, macOS).

## Compilación

Para compilar el código fuente con OpenMP, utiliza el siguiente comando:

```bash
gcc -o main main.c -fopenmp
```
```bash
./main
