from flask import Flask, request, jsonify, send_from_directory
import os
import sqlite3
from datetime import datetime

app = Flask(__name__)

# Diretório para salvar as imagens
IMAGEM_DIR = 'imagens'
os.makedirs(IMAGEM_DIR, exist_ok=True)

# Inicialização do banco
def init_db():
    conn = sqlite3.connect('banco.db')
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS registros (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    timestamp TEXT,
                    camId INTEGER,
                    caminho TEXT
                )''')
    conn.commit()
    conn.close()

init_db()

@app.route('/upload', methods=['POST'])
def upload():
    if 'image' not in request.files:
        return jsonify({"erro": "Imagem não encontrada"}), 400

    timestamp = request.form.get('timestamp')
    camId = request.form.get('camId')
    imagem = request.files['image']

    if not timestamp or not camId or not imagem:
        return jsonify({"erro": "Dados incompletos"}), 400

    # Define o caminho do arquivo
    filename = f"{timestamp}_cam{camId}.jpg"
    path = os.path.join("imagens", filename)
    imagem.save(path)

    # Salva no banco
    conn = sqlite3.connect('banco.db')
    c = conn.cursor()
    c.execute("INSERT INTO registros (timestamp, camId, caminho) VALUES (?, ?, ?)",
              (timestamp, int(camId), path))
    conn.commit()
    conn.close()

    return jsonify({"status": "ok", "caminho": path}), 200


@app.route('/imagem/<filename>')
def serve_imagem(filename):
    return send_from_directory(IMAGEM_DIR, filename)

@app.route('/registros')
def listar_registros():
    conn = sqlite3.connect('banco.db')
    c = conn.cursor()
    c.execute("SELECT timestamp, camId, caminho FROM registros ORDER BY timestamp DESC")
    dados = c.fetchall()
    conn.close()

    return jsonify([
        {"timestamp": t, "camId": cid, "caminho": c}
        for t, cid, c in dados
    ])

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
