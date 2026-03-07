<script setup>
import logoSrc from '../assets/logo.png'
</script>

<template>
  <div class="logo">
    <img :src="logoSrc" alt="Rome Chords" class="logo-img" />
    <span class="logo-text">Rome Chords</span>
  </div>
</template>

<style scoped>
.logo {
  display: flex;
  flex-direction: column;
  align-items: center;
  user-select: none;
}

.logo-img {
  height: 180px;
  width: auto;
  object-fit: contain;
}

.logo-text {
  font-size: 30px;
  font-weight: 700;
  letter-spacing: 1.5px;
  text-transform: uppercase;
  /* The colors of your gradient (Left to Right) */
  background: linear-gradient(to right, #50bddb, #d78453);
  
  /* This clips the background to the shape of the text */
  -webkit-background-clip: text;
  background-clip: text;
  
  /* This makes the actual text color disappear so the background shows through */
  -webkit-text-fill-color: transparent;
  color: transparent; 
}
</style>
