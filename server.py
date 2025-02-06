import socket

# Configuración del servidor
HOST = "0.0.0.0"  # Acepta conexiones en todas las interfaces de red
PORT = 12345  # Puerto en el que el servidor escuchará conexiones entrantes

# Creación del socket del servidor
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((HOST, PORT))  # Asigna el socket al host y puerto especificados
server_socket.listen(5)  # Permite hasta 5 conexiones en espera

print(f"Servidor esperando conexiones en {HOST}:{PORT}")

while True:
    # Acepta una nueva conexión entrante
    conn, addr = server_socket.accept()
    print(f"Conexión establecida desde: {addr}")

    while True:
        try:
            # Recibe datos del cliente
            data = conn.recv(1024).decode()
            if not data:
                print(f"Cliente {addr} desconectado")
                break  # Sale del bucle interno si el cliente se desconecta

            print(f"Dato recibido: {data}")

            # Verifica el mensaje recibido y responde en consecuencia
            if "LED ENCENDIDO" in data:
                respuesta = "ACK: LED está encendido"
            elif "LED APAGADO" in data:
                respuesta = "ACK: LED está apagado"
            else:
                respuesta = "Error: Mensaje no reconocido"

            # Envía la respuesta al cliente
            conn.sendall(respuesta.encode())

        except ConnectionResetError:
            print(f"Cliente {addr} se desconectó abruptamente")
            break

    conn.close()  # Cierra la conexión del cliente, pero sigue escuchando nuevos clientes
