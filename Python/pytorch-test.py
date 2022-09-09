#!/usr/bin/env python3
import torch
import torch.nn as nn
# import torch.optim as optim
from torchvision import datasets, transforms
import matplotlib.pyplot as plt

transform = transforms.ToTensor()  # 0 to 1, sigmoid
#  transform = transforms.Compose([ # -1 to 1, tanh
#      transforms.ToTensor(),
#      transforms.Normalize((0.5), (0.5))
#  ])
mnist_data = datasets.MNIST(
    root='./data', train=True, download=True, transform=transform)
data_loader = torch.utils.data.DataLoader(dataset=mnist_data,
                                          batch_size=64,
                                          shuffle=True)


class Autoencoder_linear(nn.Module):
    def __init__(self):
        super().__init__()
        # N, 784
        self.encoder = nn.Sequential(
            nn.Linear(28*28, 128),  # N,784 -> N,128
            nn.ReLU(),
            nn.Linear(128, 64),
            nn.ReLU(),
            nn.Linear(64, 12),
            nn.ReLU(),
            nn.Linear(12, 3)  # N, 3
        )

        # switch the sizes
        self.decoder = nn.Sequential(
            nn.Linear(3, 12),
            nn.ReLU(),
            nn.Linear(12, 64),
            nn.ReLU(),
            nn.Linear(64, 128),
            nn.ReLU(),
            nn.Linear(128, 28*28),
            nn.Sigmoid()  # get values in 0-1
        )

    def forward(self, x):
        encoded = self.encoder(x)
        decoded = self.decoder(encoded)
        return decoded


class Autoencoder_cnn(nn.Module):
    def __init__(self):
        super().__init__()
        widths = [1, 2, 4, 8]

        # N, 1, 28, 28
        self.encoder = nn.Sequential(
            nn.Conv2d(widths[0], widths[1], 3, stride=2, padding=1),
            # N, 16, 14, 14
            nn.ReLU(),
            nn.Conv2d(widths[1], widths[2], 3, stride=2, padding=1),
            # N, 32, 7, 7
            nn.ReLU(),
            nn.Conv2d(widths[2], widths[3], 7),
            # N, 64, 1, 1
        )

        # N, 64, 1, 1
        self.decoder = nn.Sequential(
            nn.ConvTranspose2d(widths[3], widths[2], 7),
            # N, 32, 7, 7
            nn.ReLU(),
            nn.ConvTranspose2d(
                widths[2], widths[1], 3,
                stride=2, padding=1, output_padding=1),
            # N, 16,14,14
            nn.ReLU(),
            nn.ConvTranspose2d(
                widths[1], widths[0], 3,
                stride=2, padding=1, output_padding=1),
            # N, 1, 28, 28
            nn.Sigmoid()  # get values in 0-1
        )

    def forward(self, x):
        encoded = self.encoder(x)
        decoded = self.decoder(encoded)
        return decoded


model = Autoencoder_cnn()
criterion = nn.MSELoss()
optimizer = torch.optim.Adam(model.parameters(), lr=3e-3, weight_decay=2e-5)

num_epochs = 13
outputs = []
for epoch in range(num_epochs):
    for (img, _) in data_loader:
        # img = img.reshape(-1, 28*28)  # for linear network
        recon = model(img)
        loss = criterion(recon, img)

        optimizer.zero_grad()
        loss.backward()
        optimizer.step()

    print(f'Epoch:{epoch+1}, Loss:{loss.item():.4f}')
    outputs.append((epoch, img, recon))


for k in range(0, num_epochs, 4):
    plt.figure(figsize=(9, 2))
    plt.title(f'Epoch {k+1}')
    plt.gray()
    imgs = outputs[k][1].detach().numpy()
    recon = outputs[k][2].detach().numpy()
    for i, item in enumerate(imgs):
        if i >= 9:
            break
        plt.subplot(2, 9, i+1)
        # item = item.reshape(-1, 28, 28)  # for linear network
        plt.imshow(item[0])

    for i, item in enumerate(recon):
        if i >= 9:
            break
        plt.subplot(2, 9, 9+i+1)
        # item = item.reshape(-1, 28, 28)  # for linear network
        plt.imshow(item[0])

plt.show()
