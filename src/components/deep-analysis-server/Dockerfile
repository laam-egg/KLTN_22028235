FROM python:3.12

WORKDIR /app

RUN apt update -y
RUN apt install git -y
RUN apt install build-essential -y

# Install dependencies first (layer cache)
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy application code
COPY . .

# Uploaded files live in a volume
RUN mkdir -p /app/uploads

EXPOSE 8000

CMD ["uvicorn", "main.__main__:app", "--host", "0.0.0.0", "--port", "8000"]
