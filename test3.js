class Vehicle {
    wheels = 4;
    virtual sound() { return "vroom vroom vroom"; }
}

class Mazda extends Vehicle {
    doors = 4;
    make = 1;
    model = 3;
    sound() { return "zoom zoom zoom"; }
}

function main() {
    var v = new Mazda();
    v.sound();
}
