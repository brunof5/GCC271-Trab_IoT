from flask import Flask, render_template_string, request, jsonify, send_from_directory
import cv2
import os
import sqlite3
from datetime import datetime

app = Flask(__name__)

# Diretório para salvar as imagens
IMAGEM_DIR = 'imagens'
SUS_IMAGEM_DIR = 'imagens_suspeitas'
os.makedirs(IMAGEM_DIR, exist_ok=True)
os.makedirs(SUS_IMAGEM_DIR, exist_ok=True)

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

    imagens = sorted(
        [f for f in os.listdir(IMAGEM_DIR) if f.endswith((".jpg", ".png"))],
        reverse=True
    )
    img1 = os.path.join(IMAGEM_DIR, imagens[1])
    img2 = os.path.join(IMAGEM_DIR, imagens[0])  # mais recente

    houve_movimento, score = detectar_movimento(img1, img2)

    if houve_movimento and score > 4000000:
        path = os.path.join(SUS_IMAGEM_DIR, filename)
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

@app.route("/galeria")
def galeria():
    imagens = os.listdir(IMAGEM_DIR)
    imagens = [img for img in imagens if img.lower().endswith((".jpg", ".jpeg", ".png"))]
    html = """
    <!DOCTYPE html>
    <html>
    <head>
        <title>Galeria de Imagens</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                text-align: center;
                padding: 20px;
            }
            .galeria {
                display: flex;
                flex-wrap: wrap;
                justify-content: center;
                gap: 15px;
            }
            .galeria img {
                width: 300px;
                height: auto;
                border: 2px solid #ccc;
                border-radius: 8px;
                transition: transform 0.2s;
            }
            .galeria img:hover {
                transform: scale(1.05);
                border-color: #333;
            }
        </style>
    </head>
    <body>
        <h1>Galeria de Imagens</h1>
        <div class="galeria">
            {% for imagem in imagens %}
                <img src="/imagem/{{ imagem }}" alt="{{ imagem }}">
            {% endfor %}
        </div>
    </body>
    </html>
    """
    return render_template_string(html, imagens=imagens)

@app.route("/galeria_suspeitas")
def galeriaSUS():
    imagens = os.listdir(SUS_IMAGEM_DIR)
    imagens = [img for img in imagens if img.lower().endswith((".jpg", ".jpeg", ".png"))]
    html = """
    <!DOCTYPE html>
    <html>
    <head>
        <title>Galeria de Imagens</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                text-align: center;
                padding: 20px;
            }
            .galeria {
                display: flex;
                flex-wrap: wrap;
                justify-content: center;
                gap: 15px;
            }
            .galeria img {
                width: 300px;
                height: auto;
                border: 2px solid #ccc;
                border-radius: 8px;
                transition: transform 0.2s;
            }
            .galeria img:hover {
                transform: scale(1.05);
                border-color: #333;
            }
        </style>
    </head>
    <body>
        <h1>Galeria de Imagens</h1>
        <div class="galeria">
            {% for imagem in imagens %}
                <img src="/imagem/{{ imagem }}" alt="{{ imagem }}">
            {% endfor %}
        </div>
    </body>
    </html>
    """
    return render_template_string(html, imagens=imagens)

def detectar_movimento(imagem1_path, imagem2_path, threshold=3000000):
    img1 = cv2.imread(imagem1_path)
    img2 = cv2.imread(imagem2_path)

    if img1 is None or img2 is None:
        return False, 0

    img1_gray = cv2.cvtColor(img1, cv2.COLOR_BGR2GRAY)
    img2_gray = cv2.cvtColor(img2, cv2.COLOR_BGR2GRAY)

    diff = cv2.absdiff(img1_gray, img2_gray)
    _, thresh = cv2.threshold(diff, 25, 255, cv2.THRESH_BINARY)

    movimento = cv2.countNonZero(thresh)
    return movimento > threshold, movimento

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
