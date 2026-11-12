from flask import Flask, request, jsonify, render_template_string
from datetime import datetime

app = Flask(__name__)

reaction_data = []

@app.route('/api/score', methods=['POST'])
def receive_score():
    data = request.get_json()
    if not data or 'reaction_time' not in data:
        return jsonify({"status": "error", "message": "Invalid payload"}), 400

    score = data['reaction_time']
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    record = {
        "id": len(reaction_data) + 1,
        "score": score,
        "timestamp": timestamp
    }
    reaction_data.append(record)
    return jsonify({"status": "success", "recorded_data": record}), 200

@app.route('/')
def dashboard():
    total_attempts = len(reaction_data)
    if total_attempts > 0:
        scores_only = [item['score'] for item in reaction_data]
        best_score = min(scores_only)
        avg_score = round(sum(scores_only) / total_attempts, 1)
    else:
        best_score = "N/A"
        avg_score = "N/A"

    html_layout = """
    <!DOCTYPE html>
    <html>
    <head>
        <title>Arduino Reaction Time Dashboard</title>
        <style>
            body { font-family: sans-serif; margin: 30px; background: #f8f9fa; }
            .card { background: white; padding: 15px; border-radius: 8px; margin-bottom: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
            table { width: 100%; border-collapse: collapse; }
            th, td { padding: 10px; border-bottom: 1px solid #ddd; text-align: left; }
        </style>
    </head>
    <body>
        <h1>⚡ Reaction Time Dashboard</h1>
        <div class="card">
            <p><strong>Total Attempts:</strong> {{ total_attempts }}</p>
            <p><strong>Best Score:</strong> {{ best_score }} ms</p>
            <p><strong>Average Score:</strong> {{ avg_score }} ms</p>
        </div>
        <div class="card">
            <h3>Trial Log</h3>
            <table>
                <tr><th>Trial #</th><th>Timestamp</th><th>Score</th></tr>
                {% for item in attempts|reverse %}
                <tr><td>#{{ item.id }}</td><td>{{ item.timestamp }}</td><td>{{ item.score }} ms</td></tr>
                {% endfor %}
            </table>
        </div>
    </body>
    </html>
    """
    return render_template_string(html_layout, total_attempts=total_attempts, best_score=best_score, avg_score=avg_score, attempts=reaction_data)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)