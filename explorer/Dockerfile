FROM python:3.11-slim
WORKDIR /app
COPY explorer/requirements.txt /app/requirements.txt
RUN pip install --no-cache-dir -r requirements.txt
COPY explorer /app
ENV FLASK_APP=app.py
CMD ["gunicorn", "-b", "0.0.0.0:8080", "app:create_app()"]
