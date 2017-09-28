module Display() {
    cube([24, 42, 10]);
    translate([4.5, 6.25, 0]) {
        cube([15, 35, 20]);
    }
}

module RTC() {
    cube([30, 30, 12]);    
}

module Board() {
    union() {
        cube([51, 71, 12]);
        
        /* Transistor. */
        translate([0, 43, 0]) {
            cube([8, 12, 25]);
        }
        
        /* Power reg. */
        translate([42, 53, 0]) {
            cube([9, 12, 25]);
        }
        
        /* Crystal. */
        translate([4, 28, 0]) {
            cube([12, 10, 20]);
        }
        
        /* Button. */
        translate([26, 4, 0]) {
            cube([7, 7, 100]);
        }
        
        /* Led path. */
        translate([-100, 42, 0]) {
            cube([100, 20, 8]);
        }  
        
        /* Power path. */
        translate([50, 54, 0]) {
            cube([100, 16, 8]);
        }
    }
}

module Elec() {
    union() {
        translate([2, 26, 0]) {
            Board();
        }
        
        translate([2+45, 26+13, 0]) {
            RTC();
        }
        
        Display();
    }
}

module Internal() {
    translate([15, 5, 3]) {
        Elec();
    }
    
    /* Cut out to save on material. */
    translate([40, 5, 3]) {
        cube([50, 20, 13]);
    }
    
    /* Bolts */
    translate([80, 35, 0]) union() {
        cylinder(r=2.5, h=20);
        cylinder(r=5, h=3);
        translate([0, 0, 17]) {
            cylinder(r=5, h=3);
        }
    }
    
    translate([8, 55, 0]) union() {
        cylinder(r=2.5, h=20);
        cylinder(r=5, h=3);
        translate([0, 0, 17]) {
            cylinder(r=5, h=3);
        }
    }  
}

/* Bottom. */
difference() {
    cube([98, 108, 10]);
    Internal();
}

/* Top. */
translate([0, 112, 0]) difference() {
    cube([98, 108, 10]);
    rotate([0, 180, 0]) 
        translate([-98, 0, -20])
        Internal();
}

/* Button. */
translate([0, 222, 0])
    cube([5, 6, 6]);