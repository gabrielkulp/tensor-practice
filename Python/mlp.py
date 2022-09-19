#!/usr/bin/env python3
import torch
import torch.nn as nn
from torchvision import datasets, transforms
import matplotlib.pyplot as plt
import numpy as np
from contraction import Contraction

# samples grabbed by the iterator in one for loop iteration
batch_size = 64
# percentage of training set to use as validation
valid_size = 0.2

num_epochs = 3
epoch_size = 64000000
disp_count = 4

transform = transforms.ToTensor()
train_data = datasets.MNIST(
    root='./data', train=True, download=True, transform=transform)
test_data = datasets.MNIST(
    root='./data', train=False, download=True, transform=transform)

num_train = len(train_data)
indices = list(range(num_train))
np.random.shuffle(indices)
split = int(np.floor(valid_size * num_train))
train_index, valid_index = indices[split:], indices[:split]

# define samplers for obtaining training and validation batches
train_sampler = torch.utils.data.sampler.SubsetRandomSampler(train_index)
valid_sampler = torch.utils.data.sampler.SubsetRandomSampler(valid_index)

# prepare data loaders
train_loader = torch.utils.data.DataLoader(
    train_data, batch_size=batch_size, sampler=train_sampler)
valid_loader = torch.utils.data.DataLoader(
    train_data, batch_size=batch_size, sampler=valid_sampler)
test_loader = torch.utils.data.DataLoader(
    test_data, batch_size=batch_size)


class MLP(nn.Module):
    def __init__(self):
        super().__init__()
        nodes = [28*28, 512, 256, 10]
        self.classifier = nn.Sequential(
            # input layer
            nn.Linear(*nodes[:2]),
            # nn.ReLU(),  # no need to normalize inputs

            # hidden layer 1
            Contraction(*nodes[1:3]),
            nn.ReLU(),
            nn.Dropout(),  # helps overfitting apparently?

            # hidden layer 2 and normalized outputs
            nn.Linear(*nodes[2:4]),
            nn.ReLU(),
        )

    def forward(self, x):
        return self.classifier(x.reshape(-1, 28*28))


model = MLP()
criterion = nn.CrossEntropyLoss()
optimizer = torch.optim.SGD(model.parameters(), lr=0.01)


# initialize tracker for minimum validation loss
# set initial "min" to infinityfor epoch in range(n_epochs):
valid_loss_min = np.Inf
# monitor losses
train_loss = 0
valid_loss = 0

training = True
if (training):
    for epoch in range(num_epochs):
        # monitor losses
        train_loss = 0
        valid_loss = 0

        ###################
        # train the model #
        ###################
        model.train()  # prep model for training
        for data, label in train_loader:
            # clear the gradients of all optimized variables
            optimizer.zero_grad()
            # forward pass: compute predicted outputs by passing inputs
            output = model(data)
            # calculate the loss
            loss = criterion(output, label)
            # backward pass: compute gradient of the loss wrt parameters
            loss.backward()
            # perform a single optimization step (parameter update)
            optimizer.step()
            # update running training loss
            train_loss += loss.item() * data.size(0)

        ######################
        # validate the model #
        ######################
        model.eval()  # prep model for evaluation
        for data, label in valid_loader:
            # forward pass: compute predicted outputs by passing inputs
            output = model(data)
            # calculate the loss
            loss = criterion(output, label)
            # update running validation loss
            valid_loss = loss.item() * data.size(0)

        # print training/validation statistics
        # calculate average loss over an epoch
        train_loss = train_loss / len(train_loader.sampler)
        valid_loss = valid_loss / len(valid_loader.sampler)

        print(f'Epoch: {epoch+1}  '
              + f'Training Loss: {train_loss:.6f}  '
              + f'Validation Loss: {valid_loss:.6f}')

        # save model if validation loss has decreased
        if valid_loss <= valid_loss_min:
            print('Validation loss decreased '
                  + f'({valid_loss_min:.6f} --> {valid_loss:.6f}). '
                  + 'Saving model...')
            torch.save(model.state_dict(), 'model.pt')
            valid_loss_min = valid_loss

model.load_state_dict(torch.load('model.pt'))

# initialize lists to monitor test loss and accuracy
test_loss = 0.0
class_correct = list(0. for i in range(10))
class_total = list(0. for i in range(10))

model.eval()  # prep model for evaluation
for data, target in test_loader:
    # forward pass: compute predicted outputs by passing inputs to the model
    output = model(data)
    # calculate the loss
    loss = criterion(output, target)
    # update test loss
    test_loss += loss.item() * data.size(0)
    # convert output probabilities to predicted class
    _, pred = torch.max(output, 1)
    # compare predictions to true label
    correct = np.squeeze(pred.eq(target.data.view_as(pred)))
    # calculate test accuracy for each object class
    for i in range(len(target)):
        label = target.data[i]
        class_correct[label] += correct[i].item()
        class_total[label] += 1  # calculate and print avg test loss
test_loss = test_loss/len(test_loader.sampler)
print(f'Test Loss: {test_loss:.6f}\n')
for i in range(10):
    if class_total[i] > 0:
        print('Test Accuracy of %5s: %2d%% (%2d/%2d)' % (
            str(i), 100 * class_correct[i] / class_total[i],
            np.sum(class_correct[i]), np.sum(class_total[i])))
    else:
        print('Test Accuracy: N/A (no training examples)')
        print('\nTest Accuracy (Overall): %2d%% (%2d/%2d)' % (
            100. * np.sum(class_correct) / np.sum(class_total),
            np.sum(class_correct), np.sum(class_total)))

# obtain one batch of test images
dataiter = iter(test_loader)
images, labels = dataiter.next()  # get sample outputs
output = model(images)
# convert output probabilities to predicted class
_, preds = torch.max(output, 1)
# prep images for display
# plot the images in the batch, along with predicted and true labels
images = images.numpy()
fig = plt.figure(figsize=(25, 4))
for idx in np.arange(20):
    ax = fig.add_subplot(2, int(20/2), idx+1, xticks=[], yticks=[])
    ax.imshow(np.squeeze(images[idx]), cmap='gray')
    title = f"{preds[idx].item()} ({labels[idx].item()})"
    color = ("green" if preds[idx] == labels[idx] else "red")
    ax.set_title(title, color=color)
plt.show()

exit(0)

outputs = []
for epoch in range(num_epochs):
    plt.figure(figsize=(10, 2))
    plt.gray()
    for i, (img, label) in zip(range(epoch_size), train_loader):
        img = img.reshape(-1, 28*28)  # linearize
        output = model(img)
        loss = criterion(output, label)

        optimizer.zero_grad()
        loss.backward()
        optimizer.step()
        if i == disp_count-1:
            print(f'Epoch:{epoch+1}, Loss:{loss.item():.4f}')
        if i < disp_count:
            plt.subplot(disp_count, 2, 2*i+1)
            plt.gca().get_xaxis().set_visible(False)
            plt.gca().get_yaxis().set_visible(False)
            plt.imshow(img.reshape(-1, 28, 28)[0])  # un-linearize

            classification = output[0].detach().numpy()
            plt.subplot(disp_count, 2, 2*i+2)
            plt.bar([x for x in range(10)], classification)
    plt.show()
